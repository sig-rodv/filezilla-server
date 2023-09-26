#include "client.hpp"
#include "client/challenger.hpp"
#include "../logger/scoped.hpp"

namespace fz::acme
{

template <typename Event, typename... Args>
std::enable_if_t<std::is_constructible_v<Event, Args...>>
client::stop(Args &&... args)
{
	assert(target_handler_ != nullptr);

	challenger_.reset();
	target_handler_->send_event<Event>(std::forward<Args>(args)...);
	d_.reset();
	stop();
}

bool client::stop_if(bool err, std::string_view msg)
{
	if (!err)
		return true;

	if (!msg.empty())
		logger_.log_u(logmsg::error, L"%s", msg);

	stop<error_event>(opid_, std::string(msg));

	return false;
}

bool client::stop_if(std::string_view err, std::string_view msg)
{
	if (err.empty())
		return true;

	std::string ret(err);

	if (!msg.empty())
		ret.append(": ").append(msg);

	logger_.log_u_raw(logmsg::error, ret);

	stop<error_event>(opid_, std::move(ret));

	return false;
}

#define STOP_IF(R, pred, msg) \
if (!stop_if(pred, msg))      \
		return R(false)       \
/***/

client::client(thread_pool &pool, event_loop &loop, logger_interface &logger, tls_system_trust_store &trust_store)
	: pool_(pool)
	, loop_(loop)
	, logger_(logger, "ACME")
	, http_client_(
		pool, loop, logger_,
		http::client::options()
			.trust_store(&trust_store)
			.follow_redirects(true)
	)
{}

client::~client()
{}

client::opid_t client::get_terms_of_service(const fz::uri &directory, event_handler &target_handler)
{
	if (!update_opid())
		return 0;

	target_handler_ = &target_handler;
	d_.emplace();

	d_->directory_uri_ = directory;
	execute(&client::get_terms_of_service_impl);

	return opid_;
}

bool client::get_terms_of_service_impl()
{
	logger_.log_u(logmsg::debug_info, L"Getting terms of service...");

	http_client_.perform("GET", d_->directory_uri_).and_then(handle([&](auto &&res){
		stop<terms_of_service_event>(opid_, fz::json::parse(res.body)["meta"]["termsOfService"].string_value());
	}));

	return false;
}

client::opid_t client::get_account(const fz::uri &directory, const std::vector<std::string> &contacts, const std::pair<json, json> &jwk, bool already_existing, event_handler &target_handler)
{
	if (!update_opid())
		return 0;

	target_handler_ = &target_handler;
	d_.emplace();

	d_->directory_uri_ = directory;
	d_->contacts_ = contacts;
	d_->jwk_ = jwk;
	d_->already_existing_account_ = already_existing;

	execute(&client::get_account_impl);

	return opid_;
}

bool client::get_account_impl()
{
	if (!execute(&client::do_get_account))
		return false;

	stop<account_event>(opid_, std::move(d_->directory_uri_), std::move(d_->kid_), std::move(d_->jwk_), std::move(d_->account_info_));
	return true;
}

client::opid_t client::get_certificate(const fz::uri &directory, const std::pair<json, json> &jwk, const std::vector<std::string> &hosts, const serve_challenges::how &how_to_serve_challenges, fz::duration allowed_max_server_time_difference, event_handler &target_handler)
{
	if (!update_opid())
		return 0;

	target_handler_ = &target_handler;
	d_.emplace();

	d_->directory_uri_ = directory;
	d_->jwk_ = jwk;
	d_->already_existing_account_ = true;
	d_->hosts_ = hosts;
	d_->allowed_max_server_time_difference_ = allowed_max_server_time_difference;

	if (auto how = std::get_if<serve_challenges::externally>(&how_to_serve_challenges))
		challenger_ = std::make_unique<challenger::external>(*how);
	else
	if (auto how = std::get_if<serve_challenges::internally>(&how_to_serve_challenges))
		challenger_ = std::make_unique<challenger::internal>(pool_, loop_, logger_, *how);
	else {
		logger_.log_u(logmsg::error, L"Don't know how to serve the challenge.");
		return 0;
	}

	d_->account_auths_.clear();
	d_->http_01_challenges_.clear();
	d_->current_account_auths_polling_ = 0;

	execute(&client::get_certificate_impl);

	return opid_;
}

bool client::get_certificate_impl()
{
	if (!execute(&client::do_get_certificate))
		return false;

	stop<certificate_event>(opid_, std::move(d_->directory_uri_), std::move(d_->kid_), std::move(d_->hosts_), std::move(d_->certificate_key_), std::move(d_->certificate_chain_));
	return true;
}

bool client::do_get_directory()
{
	if (d_->directory_)
		return true;

	logger_.log_u(logmsg::debug_info, L"Getting directory...");

	http_client_.perform("GET", d_->directory_uri_).and_then(handle([&](auto &&res){
		d_->directory_ = fz::json::parse(res.body);
		STOP_IF(void, !d_->directory_, "dd");

		logger_.log_u(logmsg::debug_info, L"Directory: %s", res.body.to_view());
	}));

	return false;
}

bool client::do_get_nonce()
{
	if (!d_->nonce_.empty())
		return true;

	if (!execute(&client::do_get_directory))
		return false;

	auto new_nonce_uri = fz::uri(d_->directory_["newNonce"].string_value());
	STOP_IF(bool, new_nonce_uri.empty(), "New nonce URI is invalid");

	logger_.log_u(logmsg::debug_info, L"Getting Nonce...");

	http_client_.perform("HEAD", new_nonce_uri).and_then(handle([&](auto){
		STOP_IF(void, d_->nonce_.empty(), "Nonce is invalid");

		logger_.log_u(logmsg::debug_info, L"Nonce: %s", d_->nonce_);
	}));

	return false;
}

bool client::do_get_account()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (d_->account_info_) {
		logger_.log(logmsg::debug_debug, L"Already got account info. Skipping...");
		return true;
	}

	if (!execute(&client::do_get_directory))
		return false;

	if (!execute(&client::do_get_nonce))
		return false;

	fz::json payload;

	for (std::size_t i = 0; i < d_->contacts_.size(); ++i)
		payload["contact"][i] = d_->contacts_[i];

	payload["termsOfServiceAgreed"] = true;
	payload["onlyReturnExisting"] = d_->already_existing_account_;

	auto new_acct = d_->directory_["newAccount"].string_value();
	STOP_IF(bool, new_acct.empty(), "New account URI is invalid");

	auto jws = make_jws(new_acct, payload, d_->nonce_, d_->jwk_, {});
	STOP_IF(bool, !jws, "get_account: couldn't generate JWS");

	logger_.log_u(logmsg::debug_info, L"Getting account...");

	http_client_.perform("POST", fz::uri(new_acct), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
		d_->account_info_ = fz::json::parse(res.body);

		if (auto it = res.headers.find("Location"); it != res.headers.end())
			d_->kid_ = it->second;
		else
			d_->kid_.clear();

		d_->already_existing_account_ = res.code == 200;

		STOP_IF(void, !d_->account_info_ || d_->kid_.empty(), "Invalid account info");

		logger_.log_u(logmsg::debug_info, L"Account object: %s", res.body.to_view());
		logger_.log_u(logmsg::debug_info, L"Account URI: %s", d_->kid_);
	}));

	return false;
}

bool client::do_get_certificate_order()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (d_->certificate_order_) {
		logger_.log(logmsg::debug_debug, L"Already got certificate order Skipping...");
		return true;
	}

	json payload;

	for (std::size_t i = 0; i < d_->hosts_.size(); ++i) {
		auto &id = payload["identifiers"][i];
		id["type"] = "dns";
		id["value"] = d_->hosts_[i];
	}

	auto new_order = d_->directory_["newOrder"].string_value();
	STOP_IF(bool, new_order.empty(), "New order URI is invalid");

	auto jws = make_jws(new_order, payload, d_->nonce_, d_->jwk_, d_->kid_);
	STOP_IF(bool, !jws, "get_certificate_order: could't generate JWS");

	logger_.log_u(logmsg::debug_info, L"Getting certificate order...");

	http_client_.perform("POST", fz::uri(new_order), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
		d_->certificate_order_ = json::parse(res.body);
		STOP_IF(void, !d_->certificate_order_, "Invalid certificate order");

		d_->certificate_order_location_.clear();
		d_->certificate_order_location_.parse(res.headers.get("Location"));
		d_->certificate_order_retry_after_ = res.headers.parse_retry_after(1);

		STOP_IF(void, !d_->certificate_order_location_, "Invalid certificate order location");

		logger_.log_u(logmsg::debug_info, L"Certificate order: %s", res.body.to_view());
	}));

	return false;
}

bool client::do_get_account_authorizations()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (const auto &status = std::as_const(d_->certificate_order_)["status"].string_value(); status != "pending") {
		logger_.log(logmsg::debug_debug, L"Certificate order status is \"%s\" rather than \"pending\". Skipping...", status);
		return true;
	}

	if (d_->account_auths_.size() == d_->hosts_.size()) {
		logger_.log(logmsg::debug_debug, L"Already got all needed account auths. Skipping...");

		return true;
	}

	auto auth_uri = d_->certificate_order_["authorizations"][d_->account_auths_.size()].string_value();

	STOP_IF(bool, auth_uri.empty(), "Invalid authorizations URI in certificate order");

	auto jws = make_jws(auth_uri, json(), d_->nonce_, d_->jwk_, d_->kid_);
	STOP_IF(bool, !jws, "get_account_authorizations: couldn't generate JWS");

	logger_.log_u(logmsg::debug_info, L"Getting account auth...");

	http_client_.perform("POST", fz::uri(auth_uri), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
		auto auth = json::parse(res.body);
		STOP_IF(void, !auth, "get_account_authorizations: invalid account authorizations");

		logger_.log_u(logmsg::debug_info, L"Account auth for [%s] is: %s", d_->hosts_[d_->account_auths_.size()], res.body.to_view());

		const auto &challenges = auth["challenges"];

		const json *http_01 = nullptr;
		for (auto &ch: challenges) {
			if (ch["type"].string_value() == "http-01") {
				http_01 = &ch;
				break;
			}
		}

		STOP_IF(void, !http_01, "No http-01 challenge found");

		d_->http_01_challenges_.push_back(*http_01);
		d_->account_auths_.push_back(std::move(auth));
	}));

	return false;
}

bool client::do_start_challenges()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (const auto &status = std::as_const(d_->certificate_order_)["status"].string_value(); status != "pending") {
		logger_.log(logmsg::debug_debug, L"Certificate order status is \"%s\" rather than \"pending\". Skipping...", status);
		return true;
	}

	if (d_->http_01_challenges_.empty()) {
		logger_.log(logmsg::debug_debug, L"All challenges have been started. Skipping...");
		return true;
	}

	const auto &ch = d_->http_01_challenges_.front();

	if (ch["status"].string_value() == "pending") {
		const auto &token = ch["token"].string_value();
		STOP_IF(bool, token.empty(), "Invalid challenge token");

		STOP_IF(bool, challenger_->serve(token, d_->jwk_), "Challenger couldn't start");

		const auto &ch_uri = ch["url"].string_value();
		STOP_IF(bool, ch_uri.empty(), "Invalid challenge URI");

		auto jws = make_jws(ch_uri, json(json_type::object), d_->nonce_, d_->jwk_, d_->kid_);
		STOP_IF(bool, !jws, "start_challenge: couldn't generate JWS");

		logger_.log_u(logmsg::debug_info, L"Starting challenge %s...", ch_uri);

		http_client_.perform("POST", fz::uri(ch_uri), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
			logger_.log_u(logmsg::debug_debug, "Challenge started: %s", res.body.to_view());
		}));
	}

	d_->http_01_challenges_.pop_front();

	return false;
}

bool client::do_wait_for_challenges_done()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (const auto &status = std::as_const(d_->certificate_order_)["status"].string_value(); status != "pending") {
		logger_.log(logmsg::debug_debug, L"Certificate order status is \"%s\" rather than \"pending\". Skipping...", status);
		return true;
	}

	json invalid_challenges;
	std::size_t num_valid = 0;
	for (const auto &a: d_->account_auths_) {
		if (a["status"].string_value() == "valid") {
			if (++num_valid == d_->account_auths_.size()) {
				logger_.log(logmsg::debug_debug, L"All challenges have been performed.");
				return true;
			}
		}
		else
		if (a["status"].string_value() == "invalid") {
			for (const auto &ch: a["challenges"]) {
				if (ch["status"].string_value() == "invalid")
					invalid_challenges[invalid_challenges.children()] = ch;
			}
		}
	}

	if (invalid_challenges) {
		const json &error = invalid_challenges[0]["error"];

		stop<error_event>(opid_, error.to_string(true));
		return false;
	}

	auto auth_uri = d_->certificate_order_["authorizations"][d_->current_account_auths_polling_].string_value();
	STOP_IF(bool, auth_uri.empty(), "Authorizations URI is invalid");

	auto jws = make_jws(auth_uri, json(), d_->nonce_, d_->jwk_, d_->kid_);
	STOP_IF(bool, !jws, "do_wait_for_challenges_done: couldn't generare JWS");

	logger_.log_u(logmsg::debug_info, L"Polling %s...", auth_uri);

	http_client_.perform("POST", fz::uri(auth_uri), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
		auto auth = json::parse(res.body);
		STOP_IF(void, !auth, "do_wait_for_challenges_done: invalid authorizations");

		logger_.log_u(logmsg::debug_info, L"Account auth for [%s] is: %s", d_->hosts_[d_->current_account_auths_polling_], res.body.to_view());
		d_->account_auths_[d_->current_account_auths_polling_] = std::move(auth);
		d_->current_account_auths_polling_ += 1;
		d_->current_account_auths_polling_ %= d_->account_auths_.size();
	}));

	return false;
}

bool client::do_finalize()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	const auto &status = std::as_const(d_->certificate_order_)["status"].string_value();

	if (status == "ready") {
		logger_.log_u(logmsg::debug_info, L"Certificate order status is \"ready\", time to finalize.");

		auto [priv, pub] = tls_layer::generate_csr({}, fz::sprintf("CN=%s", d_->hosts_[0]), d_->hosts_, false);
		d_->certificate_key_ = std::move(priv);

		auto csr = base64_encode(pub, base64_type::url, false);

		auto finalize_uri = d_->certificate_order_["finalize"].string_value();
		STOP_IF(bool, finalize_uri.empty(), "Invalid finalize URI");

		json payload;
		payload["csr"] = csr;

		auto jws = make_jws(finalize_uri, payload, d_->nonce_, d_->jwk_, d_->kid_);

		logger_.log_u(logmsg::debug_info, L"Finalizing...");

		http_client_.perform("POST", fz::uri(finalize_uri), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
			d_->certificate_order_ = json::parse(res.body);
			STOP_IF(void, !d_->certificate_order_, "Invalid certificate order");

			d_->certificate_order_location_.clear();
			d_->certificate_order_location_.parse(res.headers.get("Location"));
			d_->certificate_order_retry_after_ = res.headers.parse_retry_after(1);

			STOP_IF(void, !d_->certificate_order_location_, "Invalid certificate order location");

			logger_.log_u(logmsg::debug_info, L"New certificate order as resulting from finalize: %s", res.body.to_view());
		}));
		return false;
	}

	if (status == "valid") {
		logger_.log_u(logmsg::debug_info, L"Certificate order status is \"valid\", finalization was successful.");

		return true;
	}

	if (status == "pending" || status == "processing") {
		logger_.log_u(logmsg::debug_info, L"Certificate order status is \"%s\", polling for new status.", status);

		STOP_IF(bool, !d_->certificate_order_location_, "Invalid certificate order location");

		auto jws = make_jws(d_->certificate_order_location_.to_string(), json(), d_->nonce_, d_->jwk_, d_->kid_);

		http_client_
			.perform("POST", d_->certificate_order_location_, { { "Content-Type", "application/jose+json" } }, jws.to_string())
			.at(d_->certificate_order_retry_after_)
			.and_then(handle([&](auto &&res)
		{
			d_->certificate_order_ = json::parse(res.body);
			STOP_IF(void, !d_->certificate_order_, "Invalid certificate order");

			logger_.log_u(logmsg::debug_info, L"Current certificate order: %s", res.body.to_view());
		}));

		return false;
	}

	stop_if(std::string_view("Invalid certificate order status"), status);

	return false;
}

bool client::do_get_certificate()
{
	FZ_LOGGER_FUNCTION(logger_, logmsg::debug_debug);

	if (!d_->certificate_chain_.empty())
		return true;

	if (!execute(&client::do_get_account))
		return false;

	if (!execute(&client::do_get_certificate_order))
		return false;

	if (!execute(&client::do_get_account_authorizations))
		return false;

	if (!execute(&client::do_start_challenges))
		return false;

	if (!execute(&client::do_wait_for_challenges_done))
		return false;

	if (!execute(&client::do_finalize))
		return false;

	auto certificate_uri = d_->certificate_order_["certificate"].string_value();
	STOP_IF(bool, certificate_uri.empty(), "Invalid certificate URI");

	auto jws = make_jws(certificate_uri, json(), d_->nonce_, d_->jwk_, d_->kid_);

	logger_.log_u(logmsg::debug_info, L"Getting certificate...");

	http_client_.perform("POST", fz::uri(certificate_uri), { { "Content-Type", "application/jose+json" } }, jws.to_string()).and_then(handle([&](auto &&res){
		d_->certificate_chain_ = res.body.to_view();

		logger_.log_u(logmsg::debug_info, L"Certificate: %s", res.body.to_view());
	}));

	return false;
}

bool client::update_opid()
{
	fz::scoped_lock lock(mutex_);

	if (!opstack_.empty()) {
		logger_.log_u(logmsg::error, L"An operation is already being executed.");
		return false;
	}

	++opid_;
	if (!opid_)
		opid_ = 1;

	return true;
}

bool client::execute(operation op)
{
	fz::scoped_lock lock(mutex_);
	opstack_.push(op);

	lock.unlock();

	if (!(this->*op)())
		return false;

	lock.lock();

	// The check is there because somebody might have stopped us, i.e. emptied the stack.
	if (!opstack_.empty())
		opstack_.pop();

	return true;
}

void client::reenter()
{
	fz::scoped_lock lock(mutex_);

	while (!opstack_.empty()) {
		auto op = opstack_.top();
		lock.unlock();

		if (!(this->*op)())
			return;

		lock.lock();
		// The check is there because somebody might have stopped us, i.e. emptied the stack.
		if (!opstack_.empty())
			opstack_.pop();
	}
}

void client::stop()
{
	scoped_lock lock(mutex_);
	decltype(opstack_)().swap(opstack_);
}

http::response::handler client::handle(http::response::handler h)
{
	return [this, h=std::move(h)](http::response &&res) {
		assert(d_);

		std::string error_string;

		if (res.code_type() >= res.client_error) {
			if (auto content_type = res.headers.get("Content-Type"); content_type == "application/problem+json") {
				fz::json error = json::parse(res.body);
				if (error["type"].string_value() == "urn:ietf:params:acme:error:badNonce") {
					logger_.log_u(logmsg::debug_verbose, "Nonce is invalid or has expired. Getting a new one.");

					d_->nonce_.clear();
					execute(&client::do_get_nonce);
					return;
				}

				error_string = res.body.to_view();
			}
			else
				error_string = fz::sprintf("HTTP %s: %s", res.code_string(), res.reason);

			logger_.log_u(logmsg::error, L"Error: %s", error_string);

			stop<error_event>(opid_, error_string);
			return;
		}

		if (d_->allowed_max_server_time_difference_) {
			if (auto it = res.headers.find("Date"); it != res.headers.end()) {
				fz::datetime server_dt;
				if (!server_dt.set_rfc822(it->second))
					logger_.log_u(logmsg::debug_warning, L"Server's Date header doesn't contain a proper date.");
				else {
					auto now = datetime::now();
					auto delta_exceeded =
						(server_dt < now && (now-server_dt) > d_->allowed_max_server_time_difference_) ||
						(now < server_dt && (server_dt-now) > d_->allowed_max_server_time_difference_);

					if (delta_exceeded) {
						const static std::string err = "ACME server's date and " PACKAGE_NAME "'s date differ too much.";
						logger_.log_u(logmsg::error, err);
						stop<error_event>(opid_, err);
						return;
					}
				}
			}
		}

		STOP_IF(void, res.code_type() != res.successful, "Unexpected HTTP code");

		if (auto it = res.headers.find("Replay-Nonce"); it != res.headers.end())
			d_->nonce_ = it->second;
		else
			d_->nonce_.clear();

		if (h)
			h(std::move(res));

		reenter();
	};
}

fz::json client::make_jws(const std::string &url, const fz::json &payload, const std::string &nonce, const std::pair<fz::json, fz::json> &jwk, const std::string &kid)
{
	fz::json extra;

	extra["url"] = url;

	if (!nonce.empty())
		extra["nonce"] = nonce;

	if (kid.empty())
		extra["jwk"] = jwk.second;
	else
		extra["kid"] = kid;

	logger_.log_u(logmsg::debug_debug, "make_jws, payload: %s", payload.to_string());
	logger_.log_u(logmsg::debug_debug, "make_jws, extra: %s", extra.to_string());

	return fz::jws_sign_flattened(jwk.first, payload, extra);
}

}
