#include "tlog.h"

using namespace std;
using namespace logging::trivial;
using boost::shared_ptr;

bool TLog::init_from_settingfile(std::string path)
{
//    logging::add_common_attributes();
//    logging::register_simple_formatter_factory<severity_level, char>("Severity");
//    logging::register_simple_filter_factory<severity_level, char>("Severity");

    //init_factories();
    std::ifstream file(path.c_str());
    try
    {
        logging::init_from_stream(file);
    }
    catch (const std::exception& e)
    {
        std::cout << "init_logger is fail, read log config file fail. curse: " << e.what() << std::endl;
        //exit(-2);
        return false;
    }
    return true;
}

void TLog::init(const std::string & fullname, int rotationInternal,std::string log_level)
{
    string filename_path = fullname.substr(0,fullname.find_last_of("/"));
    auto file_sink =logging::add_file_log
    (
        keywords::open_mode = std::ios::app,
        keywords::file_name = fullname,                                /*< file name pattern >*/
//      keywords::rotation_size = 1 * 1024 * 1024,                     /*< rotate files every 10 MiB... >*/
//      keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0), /*< ...or at midnight >*/
        keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::minutes(rotationInternal)),
//      keywords::format = "[%TimeStamp%]%Message%",                                      /*< log record format >*/
        keywords::target         = filename_path + "/old",
        keywords::max_size       =(uint64_t)10* 1024*1024 * 1024,                  /* all files size puls, in bytes*/
        keywords::min_free_space =(uint64_t)10*1024*1024*1024,                     /* minimum free space on the drive, in bytes*/
        keywords::auto_flush     = true
 //       keywords::filter         = expr::attr< severity_level >("Severity") >= info
    );


    file_sink->set_formatter
    (
        expr::format("%1%|<%2%>|<%3%>%4%")
        % expr::format_date_time(_timestamp, "%Y-%m-%d %H:%M:%S")
        % expr::attr< attrs::current_thread_id::value_type >("ThreadID")
        % expr::attr< severity_level >("Severity")
        % expr::smessage  
    );

    if (log_level=="debug")
    {
        file_sink->set_filter(logging::trivial::severity>=logging::trivial::debug);
    }
    if (log_level=="info")
    {
        file_sink->set_filter(logging::trivial::severity>=logging::trivial::info);
    }
    if (log_level=="warn")
    {
        file_sink->set_filter(logging::trivial::severity>=logging::trivial::warning);
    }
    if (log_level=="error")
    {
         file_sink->set_filter(logging::trivial::severity>=logging::trivial::error);
    }
    if (log_level=="fatal")
    {
         file_sink->set_filter(logging::trivial::severity>=logging::trivial::fatal);
    }


    logging::core::get()->add_sink(file_sink);

    logging::add_common_attributes();
}



void TLog::trf_log_debug(const char * format, ...){
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(debug) << "  |Y|" << buffer;
    va_end(args);
    return;
}

void TLog::trf_log_info(const char * format, ...){
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(info) << "   |Y|" << buffer;
    va_end(args);
    return;
}

void TLog::trf_log_warn(const char * format, ...){
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(warning) << "|Y|" << buffer;
    va_end(args);
    return;
}

void TLog::trf_log_error(const char * format, ...){
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(error) << "  |N|" << buffer;
    va_end(args);
    return;
}

void TLog::trf_log_fatal(const char * format, ...){
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(fatal) << buffer;
    va_end(args);
    return;
}

void TLog::trf_log_info_timeout(const char * format, ...)
{
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(info) << "   " << buffer;
    va_end(args);
    return;
}
void TLog::trf_log_error_timeout(const char * format, ...)
{
    char buffer[MAX_LOG_LEN] = {0};
    va_list args;
    va_start (args, format);
    vsnprintf(buffer, MAX_LOG_LEN, format, args);
    int idx = kmp(buffer, "\n\0");
    if(idx != -1){
        buffer[idx] = '\0';
    }
    BOOST_LOG_TRIVIAL(error) <<" "<< buffer;
    va_end(args);
    return;
}

//int test(int, char*[])
//{
//  init();
//  //init_log_environment("/home/chenxh/workspace/boostlog/src/log_setting.ini");
//
////    using namespace logging::trivial;
////    src::severity_logger< severity_level > lg;
//
//    while(1){
//    BOOST_LOG_TRIVIAL(trace) << "A trace severity message" << 000;
//    BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
//    BOOST_LOG_TRIVIAL(info) << "An informational severity message" << 111;
//    BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
//    BOOST_LOG_TRIVIAL(error) << "An error severity message" << 222;
//    BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message" << 333;
//    sleep(1);
//    }
//
//    return 0;
//}


int TLog::kmp(const char *text, const char *find)
{
    if (text == '\0' || find == '\0')
        return -1;
    int find_len = strlen(find);
    int text_len = strlen(text);
    if (text_len < find_len)
        return -1;
    int map[find_len+10];
    memset(map, 0, (find_len+10)*sizeof(int));
    //initial the kmp base array: map
    map[0] = 0;
    map[1] = 0;
    int i = 2;
    int j = 0;
    for (i=2; i<find_len; i++)
    {
        while (1)
        {
            if (find[i-1] == find[j])
            {
                j++;
                if (find[i] == find[j])
                {
                    map[i] = map[j];
                }
                else
                {
                    map[i] = j;
                }
                break;
            }
            else
            {
                if (j == 0)
                {
                    map[i] = 0;
                    break;
                }
                j = map[j];
            }
        }
    }
    i = 0;
    j = 0;
    for (i=0; i<text_len;)
    {
        if (text[i] == find[j])
        {
            i++;
            j++;
        }
        else
        {
            j = map[j];
            if (j == 0)
                i++;
        }
        if (j == (find_len))
            return i-j;
    }
    return -1;
}

