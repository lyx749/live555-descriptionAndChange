#ifndef __PROCESS_MUTEX_HPP
#define __PROCESS_MUTEX_HPP
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>
#include <string>
#include <execinfo.h>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <memory>
#define ASSERT(X)                                                    \
    if (!X)                                                          \
    {                                                                \
        std::cerr << "ASSERTION: " #X                                \
                  << "\nbacktrace: \n"                               \
                  << process_mutex::BacktraceToString(100, 2, "  "); \
        assert(X);                                                   \
    }

namespace process_mutex
{
#define SHARED_MEMORY_SIZE 20000
#define GET_KEY ftok("./", 200)

    static void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);
    static std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

    template <class T, class X = void, int N = 0>
    class Singleton
    {
    public:
        static std::shared_ptr<T> GetInstance()
        {
            static std::shared_ptr<T> v(new T());
            return v;
        }
    };

    class ProcessMutexLock;
    class ProcessMutex
    {
        friend class ProcessMutexLock;

    private:
        ProcessMutex(){};
        void ProcessMutexInit()
        {
            ASSERT(pthread_mutexattr_init(&m_lock_attr) == 0);
            ASSERT(pthread_mutexattr_setpshared(&m_lock_attr, PTHREAD_PROCESS_SHARED) == 0);
            ASSERT(pthread_mutex_init(&m_lock, &m_lock_attr) == 0);
        };
        void ProcessMutexDestroy()
        {
            ASSERT(pthread_mutexattr_destroy(&m_lock_attr) == 0);
            ASSERT(pthread_mutex_destroy(&m_lock) == 0);
        }

    private:
        pthread_mutex_t m_lock;
        pthread_mutexattr_t m_lock_attr;
    };

    class ProcessMutexLock
    {
        template <class, class, int>
        friend class Singleton;

    private:
        ProcessMutexLock()
        {
            key_t key = GET_KEY;
            flag = shmget(key, sizeof(ProcessMutex), IPC_CREAT | 0x776);
            ASSERT(flag < 0);
            m_Mutex = (ProcessMutex *)shmat(flag, NULL, SHM_R | SHM_W);
            ASSERT(m_Mutex != NULL);
            m_Mutex->ProcessMutexInit();
        }

    public:
        ~ProcessMutexLock()
        {
            m_Mutex->ProcessMutexDestroy();
            ASSERT(shmctl(flag, IPC_RMID, NULL) == 0);
        }

        void lock()
        {
            pthread_mutex_lock(&m_Mutex->m_lock);
        }

        void unlock()
        {
            pthread_mutex_unlock(&m_Mutex->m_lock);
        }

    private:
        ProcessMutex *m_Mutex;
        int flag;
    };

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc((sizeof(void *) * size));
        size_t s = ::backtrace(array, size);

        char **strings = backtrace_symbols(array, s);
        if (strings == NULL)
        {
            return;
        }
        for (size_t i = skip; i < s; ++i)
        {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

}
#define PROCESS_MUTEX process_mutex::Singleton<process_mutex::ProcessMutexLock>::GetInstance()
#endif