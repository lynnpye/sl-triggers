#include "contexting.h"

#include "threadmanager.h"

static ContextManager* g_ContextManager = nullptr;

OnAfterSKSEInit([]{
    g_ContextManager = &ContextManager::GetSingleton();
});

std::shared_ptr<ThreadContext> ContextManager::CreateThreadContext(RE::TESForm* target, std::string initialScriptName) {
    if (target == nullptr)
        return nullptr;
    
    TargetContext* targetContext = GetTargetContext(target);

    if (!targetContext) {
        targetContext = CreateTargetContext(target);
    }

    std::shared_ptr<ThreadContext> newThreadContext = targetContext->CreateThreadContext(initialScriptName);
    // now I need to hand this off or notify a ThreadManager to start doing something?


    return newThreadContext;
}

std::shared_ptr<ThreadContext> TargetContext::CreateThreadContext(std::string initialScriptName) {
    if (initialScriptName.empty())
        return nullptr;
    
    std::shared_ptr<ThreadContext> newThreadContext = WithThreads([papa = this, initialScriptName](auto& threads){
        auto newPtr = std::make_shared<ThreadContext>(papa, initialScriptName);
        threads.push_back(newPtr);
        return newPtr;
    });

    ThreadManager::GetSingleton().RegisterContext(newThreadContext);

    return newThreadContext;
}

void ThreadContext::Execute() {
    while (!shouldStop && !callStack.empty()) {
        while (!shouldStop && callStack.back()->RunStep()) {
            // twiddle my thumbs? actually, should take these moments to check for things like
            // requests to shut down, save state, etc.
        }
        if (!shouldStop) {
            callStack.pop_back();
        }
    }


    if (!shouldStop) {
        // I am stopping because my callStack is empty
        // clean myself up, request my removal, shut down my real-thread
    } else {
        // I am stopping against my will
    }
}


// returns false when the FrameContext has nothing else to run
// i.e. end of script or final return
bool FrameContext::RunStep() {
/*
    currentLine initializes to 0, so assume correct state coming in
    so we should always advance it before exiting
    and if it is equal to or greater than scriptTokens.size() we return false

    otherwise we try to run the line of code, skipping any that are unparseable or unrunnable
    need to capture errors as well to output
    perhaps make those functions/events/etc.

        line[0], line[1]...
        perform same basic logic as currently in Papyrus... i.e. check line[0] for the operation
        many things can be handled directly
        others have to be handed off

*/
    return false;
}