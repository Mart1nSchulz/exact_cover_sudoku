#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>

template<typename T>
class BlockingQueue
{
private:
    std::mutex mut;
    std::deque<T> private_std_queue;
    std::condition_variable condNotEmpty;
    std::condition_variable condNotFull;
    int count; // Guard with Mutex
	int maxSize;
public:
	BlockingQueue(int max) {
		maxSize = max;
	}

	template<typename... Args>
    void emplace_back(Args&&... args) {
        std::unique_lock<std::mutex> lk(mut);
        //Condition takes a unique_lock and waits given the false condition
        condNotFull.wait(lk,[this]{
            if (count== maxSize) {
                return false;
            }else{
                 return true;
            }
            
        });
       	private_std_queue.emplace_back(args...);
       	count++;
        condNotEmpty.notify_one();
	}

    void put(T new_value)
    {
        std::unique_lock<std::mutex> lk(mut);
        //Condition takes a unique_lock and waits given the false condition
        condNotFull.wait(lk,[this]{
            if (count== maxSize) {
                return false;
            }else{
                 return true;
            }
            
        });
       	private_std_queue.push_back(new_value);
       	count++;
        condNotEmpty.notify_one();
    }
    void take(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
         //Condition takes a unique_lock and waits given the false condition
        condNotEmpty.wait(lk,[this]{return !private_std_queue.empty();});
       	value=private_std_queue.front();
       	private_std_queue.pop_front();
       	count--;
        condNotFull.notify_one();
    }
    int getCount() {
		return count;
    }
};
