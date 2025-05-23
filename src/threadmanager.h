#pragma once

#include "contexting.h"

#include <thread>

class ThreadManager {
public:

    static ThreadManager& GetSingleton() {
        static ThreadManager singleton;
        return singleton;
    }

    std::mutex runningThreadsMutex;
    std::unordered_map<std::shared_ptr<ThreadContext>, std::thread> runningThreads;

    template <typename Func>
    void WithRunningThreads(Func&& fn) {
        std::lock_guard<std::mutex> lock(runningThreadsMutex);
        fn(runningThreads);
    }

    void StartAll(std::vector<std::shared_ptr<ThreadContext>>& contexts) {
        for (auto ctx : contexts) {
            if (!ctx->IsTargetValid()) {
                // log/prune
                continue;
            }
            WithRunningThreads([ctx](auto& runningThreads){
                runningThreads[ctx] = std::thread([ctx] {
                    ctx->Execute();
                });
            });
        }
    }

    void StopAll() {
        WithRunningThreads([](auto& runningThreads){
            for (auto& [ctx, th] : runningThreads) {
                ctx->RequestStop(); // sets atomic flag
            }
            for (auto& [ctx, th] : runningThreads) {
                if (th.joinable()) th.join();
            }
            runningThreads.clear();
        });
    }

    void RegisterContext(std::shared_ptr<ThreadContext> ctx) {
        WithRunningThreads([ctx](auto& runningThreads){
            runningThreads[ctx] = std::thread([ctx] {
                ctx->Execute();
                // now remove the ThreadContext 
                ctx->target->RemoveThreadContext(ctx.get());
            });
        });
    }
};