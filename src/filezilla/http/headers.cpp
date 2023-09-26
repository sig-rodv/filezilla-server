#include "headers.hpp"

namespace fz::http
{

fz::datetime headers::parse_retry_after(unsigned int min_seconds_later) const
{
	datetime at;

	auto now = datetime::now();
	auto later = now + duration::from_seconds(min_seconds_later);

	if (const auto &ra = get("Retry-After"); !at.set_rfc822(ra) && ra) {
		if (auto secs = fz::to_integral<unsigned int>(ra))
			at = now + duration::from_seconds(secs);
		}

	if (at < later)
		at = later;

	return at;
}

}
