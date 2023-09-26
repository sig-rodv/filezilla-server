#ifndef FZ_RMP_ENGINE_SERVER_HPP
#define FZ_RMP_ENGINE_SERVER_HPP

#include <list>

#include <libfilezilla/event_handler.hpp>
#include <libfilezilla/logger.hpp>

#include "../../tcp/server.hpp"
#include "../../securable_socket.hpp"
#include "../../util/locking_wrapper.hpp"

#include "../../rmp/engine.hpp"
#include "../../rmp/engine/dispatcher.hpp"
#include "../../rmp/engine/session.hpp"
#include "../../rmp/engine/forwarder.hpp"
#include "../../rmp/address_info.hpp"

namespace fz::rmp {

template <typename AnyMessage>
class engine<AnyMessage>::server: private tcp::session::factory
{
public:
	server(fz::tcp::server_context &context, dispatcher &dispatcher,
		   logger_interface &logger);

	void set_listen_address_infos(const std::vector<address_info> &address_info_list);
	void set_security_info(const securable_socket::info &info);

	template <typename Message, typename... Args>
	std::enable_if_t<trait::has_message_v<any_message, Message> && std::is_constructible_v<Message, Args...>, int>
	broadcast(Args &&... args);

	template <typename Message, typename... Args>
	std::enable_if_t<trait::has_message_v<any_message, Message> && std::is_constructible_v<Message, Args...>, std::pair<int, typename session::id>>
	send_to_random_client(Args &&... args);

	//! \returns the number of sessions currently active
	std::size_t get_number_of_sessions() const;

	void start();
	void stop();

	util::locked_proxy<session> get_session(typename session::id);

	std::size_t end_sessions(const std::vector<typename session::id> &ids = {}, int err = 0);

private:
	std::unique_ptr<tcp::session> make_session(event_handler &target_handler, tcp::session::id id, std::unique_ptr<socket> socket, const std::any &user_data, int &error /* In-Out */) override;

private:
	tcp::server tcp_server_;
	dispatcher &dispatcher_;
	logger_interface &logger_;
	securable_socket::info security_info_{};
};

}

#include "../../rmp/engine/server.ipp"

#endif // FZ_RMP_ENGINE_SERVER_HPP
