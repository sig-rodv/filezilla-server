#ifndef FZ_UPDATE_INFO_RETRIEVER_FILE_HPP
#define FZ_UPDATE_INFO_RETRIEVER_FILE_HPP

#include "../../logger/modularized.hpp"

#include "../info.hpp"

namespace fz::update::raw_data_retriever
{

class file: public info::raw_data_retriever
{
public:
	file(const native_string &persist_path, logger_interface &logger);

	void retrieve_raw_data(bool manual, fz::receiver_handle<result>) override;

	void persist_if_newer(std::string_view raw_data, fz::datetime timestamp);

private:
	logger::modularized logger_;
	native_string persist_path_;
	fz::duration expiration_;
};

}

#endif // FZ_UPDATE_INFO_RETRIEVER_FILE_HPP
