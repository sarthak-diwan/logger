#pragma once

#include <chrono>
#include <fstream>
#include <cstring>
#include "concurrentqueue.h"

#define LOG(...) Logger::getInstance()->log(__VA_ARGS__)

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

#if LOG_LEVEL >= 3
#define DEBUG_LOG(...) Logger::getInstance()->log("DEBUG: " ,__VA_ARGS__)
#else
#define DEBUG_LOG(...) 
#endif

#if LOG_LEVEL >= 2
#define INFO_LOG(...) Logger::getInstance()->log("INFO: ", __VA_ARGS__)
#else
#define INFO_LOG(...) 
#endif

#if LOG_LEVEL >= 1
#define ERROR_LOG(...) Logger::getInstance()->log("ERROR: ", __VA_ARGS__)
#else
#define ERROR_LOG(...)
#endif

class Array{
    uint8_t * data;
    size_t ind=0;
    size_t pop_ind=0;
    public:
        Array() = default;
        Array(size_t size){
            data = (uint8_t *)malloc(size);
        }

        template<typename T>
        void push(T && t){
            memcpy(data + ind, &t, sizeof(t));
            ind += sizeof(t);
        }

        template<typename T>
        T pop(){
            T x = *reinterpret_cast<T*>(data + pop_ind);
            pop_ind += sizeof(T);
            return x;
        }

        void clear(){
            free(data);
        }

};

class Logger {
    moodycamel::ConcurrentQueue<Array> m_q;
    std::thread m_t;
    inline static std::ofstream file_;
    std::atomic<bool> m_exit;

    Logger(){
        m_exit = false;
        m_t = std::thread([this]()->void{
            file_.open("log.txt");
            Array a;
            do{
                while(m_q.try_dequeue<Array>(a)){
                    auto f = a.pop<void(*)(Array &)>();
                    (*f)(a);
                }
                file_.flush();
            }while(!m_exit);
        });
    }

    public:
        Logger(Logger const&) = delete;
        Logger& operator=(Logger const&) = delete;
        ~Logger(){
            m_exit=true;
            m_t.join();
        };

        static Logger * getInstance(){
            static Logger instance{};
            return &instance;
        }

        template<typename ...Args>
        void log(Args ... args ){
            size_t total_size = (... + sizeof(args)) + sizeof(void(*)(Array &)) + sizeof(std::chrono::_V2::system_clock::time_point);
            Array v(total_size);
            v.push(&output<Args...>);
            const auto now = std::chrono::system_clock::now();
            v.push(now);
            (..., v.push(args));
            m_q.enqueue(v);
        }

        template<typename ...Args>
        static void output(Array &a){
            const auto now = a.pop<std::chrono::_V2::system_clock::time_point>();
            const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
            const auto fine = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            char buffer[sizeof "9999-12-31 23:59:59.999"];
            std::snprintf(buffer + std::strftime(buffer, sizeof buffer - 3,
                                        "%F %T.", std::localtime(&t_c)),
                4, "%03lu", fine.time_since_epoch().count() % 1000);
            file_ << buffer << ", ";
            (..., (file_ << a.pop<Args>()));
            file_ << std::endl;
            a.clear();
        }
};

