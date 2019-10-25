namespace KWShared{
    enum RamCacheItemState {unloaded, loading, loaded};
    class RamCacheItem{
        private:
            vector<function<void(string)>> startingObservers;
            function<string()> loadFunction = NULL;
            string data;
            RamCacheItemState state = RamCacheItemState::unloaded;
        public:
            string name;
            RamCacheItem(string name): name(name) { }
            unsigned int getSize(){
                return data.size() + sizeof(startingObservers) + sizeof(state) + sizeof(loadFunction);
            }

            //just first funciton will be called
            void asyncGet(function<string()> loadFunction, function<void(string)> onload){
                if (this->state == RamCacheItemState::unloaded)
                {
                    this->state = RamCacheItemState::loading;
                    startingObservers.push_back(onload);

                    this->loadFunction = loadFunction;
                    this->data = this->loadFunction();
                    this->state = RamCacheItemState::loaded;

                    for (auto &c: this->startingObservers)
                        c(this->data);

                    this->startingObservers.clear();
                }
                else if (this->state == RamCacheItemState::loading)
                    startingObservers.push_back(onload);
                else
                    onload(this->data);
            };
    };

    class RamCache
    {
        private:
            vector<RamCacheItem*> cache;
            unsigned int maxSize = 0;

            void releaseCache(){
                //calculate cacheSize
                unsigned int tsize = 0;
                for (auto &c: cache)
                    tsize+=c->getSize();

                //remove the commonly less used items
                while (tsize > this->maxSize && this->cache.size() > 0)
                {
                    tsize -= this->cache.back()->getSize();
                    delete this->cache.back();
                    cache.erase(cache.begin() + cache.size());
                }
            }
        public:
            RamCache(int maxSize):
                maxSize(maxSize)
            {

            };

            void getAsync(string resource, function<void(string)> resultFunction, function<string()> loadFunction){

                int posInCache = -1;
                for (unsigned int c = 0; c < cache.size(); c++)
                {
                    if (cache[c]->name == resource)
                    {
                        posInCache = c;
                        break;
                    }
                }

                if (posInCache == -1)
                {
                    cache.insert(cache.begin(), new RamCacheItem(resource));
                    posInCache = 0;
                }
                else
                {
                   //up curret resource in one position in the cache
                    if (posInCache > 0)
                    {
                        auto tmp = cache[posInCache];
                        cache.erase(cache.begin() + posInCache);
                        cache.insert(cache.begin() + (posInCache-1), tmp);
                    }
                }

                cache[posInCache]->asyncGet(loadFunction, [this, resultFunction](string getResult){
                    //get the totalSize
                    this->releaseCache();

                    //releases data from cache
                    resultFunction(getResult);


                });
            }

            string getSync(string resource, function<string()> loadFunction){
                string result;
                std::mutex l;
                l.lock();
                this->getAsync(resource, [&](string funcResult){
                    result = funcResult;
                    l.unlock();
                }, loadFunction);

                l.lock();
                l.unlock();

                return result;
            }

    };
}
