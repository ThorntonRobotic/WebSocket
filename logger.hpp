/* Copyright 2021 Tim Thornton
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# References
#
# History:
#
# Building
#
# TODO
*/ 

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>

#pragma once

#ifdef LOGGER_USE_STUB
class LoggerStub {
private:
    bool debug_;
    bool error_;
    bool fatal_;
    bool warn_;
    bool info_;

public:
    LoggerStub(): debug_(true), error_(true), fatal_(true), warn_(true), info_(true){};
    ~LoggerStub() = default;
    void debug(std::string  s){ if(debug_) std::cout << s << std::endl;};
    void error(std::string  s){ if(error_) std::cout << s << std::endl;};
    void fatal(std::string  s){ if(fatal_) std::cout << s << std::endl;};
    void warn(std::string  s){ if(warn_) std::cout << s << std::endl;};
    void info(std::string  s){ if(info_) std::cout << s << std::endl;};
};
#else
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#endif


// TODO:
// Change log level realtime

class Logger
{
private:
#ifdef LOGGER_USE_STUB    
    LoggerStub * logger_;
#else
    log4cxx::LoggerPtr logger_;
#endif

    std::string formatter_;

public:
    // General purpose string formatter
    // Include in this class for now. Determin whether there should be somewhere else thate makes more sense
    static std::string formatString(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string str = formatString(fmt, args);
        va_end(args);
        return str;
    }

private:
    static std::string formatString(const char *fmt, va_list args)
    { 
#ifdef __WIN32__
        char * ret = (char *) malloc(1024);
        if(vsprintf(ret, fmt, args) < 0){
            return std::string("Logger::formatString error");
        }
#else
        char *ret;
        if(vasprintf(&ret, fmt, args) < 0){
            // Something bad happened
            return std::string("Logger::formatString error");
        }
#endif

        std::string str(ret);
        free(ret);
        return str;
    }
    
public:
    Logger(std::string name, std::string properties_file)
    {
    #ifdef LOGGER_USE_STUB
        logger_ = new LoggerStub();
    #else
        log4cxx::PropertyConfigurator::configure(properties_file);
        logger_ = log4cxx::Logger::getLogger(name);
    #endif
    
    }

    void debug(std::string s) const
    {
        logger_->debug(s);
    }

    void debug(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logger_->debug(formatString(fmt, args));
        va_end(args);
    }

    void error(std::string s) const
    {
        logger_->error(s);
    }

    void error(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logger_->error(formatString(fmt, args));
        va_end(args);
    }

    void fatal(std::string s) const
    {
        logger_->fatal(s);
    }

    void fatal(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logger_->fatal(formatString(fmt, args));
        va_end(args);
    }

    void warn(std::string s) const
    {
        logger_->warn(s);
    }

    void warn(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logger_->warn(formatString(fmt, args));
        va_end(args);
    }
    void info(std::string s) const
    {
        logger_->info(s);
    }

    void info(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        logger_->info(formatString(fmt, args));
        va_end(args);
    }

};