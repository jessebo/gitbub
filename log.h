
#ifndef TLOG_H_
#define TLOG_H_

#include <iostream>
#include <fstream>
#include <boost/log/core.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/expressions/filter.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp> 

#include <boost/date_time/posix_time/posix_time.hpp>




namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
//namespace flt = boost::log::filters;



BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag2", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(_timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(_uptime, "Uptime", attrs::timer::value_type);
BOOST_LOG_ATTRIBUTE_KEYWORD(_scope, "Scope", attrs::named_scope::value_type);
//BOOST_LOG_ATTRIBUTE_KEYWORD(log_severity, "Severity", SeverityLevel);



#define MAX_LOG_LEN (10000)


class TLog{
public:
	static void init(const std::string & fullname, int rotationInternal =30,std::string level="warn");
	static bool init_from_settingfile(std::string path);
	static void trf_log_debug(const char * pstrFormat, ...);
	static void trf_log_info(const char * pstrFormat, ...);
	static void trf_log_warn(const char * pstrFormat, ...);
	static void trf_log_error(const char * pstrFormat, ...);
	static void trf_log_fatal(const char * pstrFormat, ...);
	static void trf_log_info_timeout(const char * pstrFormat, ...);
	static void trf_log_error_timeout(const char * pstrFormat, ...);
	static int kmp(const char *text, const char *find);
};

#endif 

