#include <bits/stdc++.h>
#include <unistd.h>
using namespace std;


class cache
{
public:
    cache(int size_,int expire_):n(size_),expire_time(expire_){
        times=0;
    };
    int set(int id,int val)
    {
        Object temp{val,times};
        if(storage.size()<n)
        {  
            storage[id]=temp;
            return 1;
        }
        else
        {
            int min_time=999999;
            for(auto x:storage)
            {
                if(x.second.time_stamp<min_time)
                    min_time=x.second.time_stamp;
            }
            storage.erase(min_time);
            storage[id]=temp;
        }
    };

    int get(int id)
    {
        if(storage.find(id)==storage.end())
            return -1;
        storage[id].time_stamp=times;
        return storage[id].value;
    };

    ~cache()
    {
        exit=true;
        sleep(1);
    }

private:
    struct Object{
        int value;
        int time_stamp;
    };
    map<int,Object> storage;
    int n;
    int times;
    bool exit=false;
    int expire_time;

    void tick()
    {
        while(!exit)
        {
            for(auto x:storage)
            {
                if(x.second.time_stamp-times>expire_time)
                    storage.erase(x.first);
            }
            times++;
            sleep(1);
        }
        return;
    }
};


int main() {
    cache Cache(5,3);

    Cache.set(2,10);
    Cache.set(1,100);
    Cache.set(3,1000);

    cout<<Cache.get(1)<<endl;
    Cache.set(4,8);
    cout<<Cache.get(2);

    return 0;
}
