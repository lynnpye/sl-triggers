#include "engine.h"
#include "contexting.h"

using namespace SLT;

bool Salty::ParseScript(FrameContext* frame) {
    logger::info("SLTEngineParseScript({}) - I call this a win", frame->scriptName);
    // this will go to disk, get the file using frame->scriptName, and do the parsing
    // this will populate frame->scriptTokens
    // frame->currentLine should be set to 0
    // frame->commandType should be set appropriately (and still needs conversion to enum)
    // in fact I guess all of the various members should, just in case, be set to default
    // not sure how to handle the mutexing in that case
    return false;
}

bool Salty::IsStepRunnable(std::vector<std::string>& step) {
    // probably going to make an engine.cpp and move this stuff there but
    // this is where real script emulation will start happening
    // but, to give an idea of the point of this, this would check for things like
    // empty lines
    // lines that are only comments
    // lines that are just plainly wrong
    return false;
}

bool Salty::RunStep(std::vector<std::string>& step) {
    // again, will probably be pushed to engine.cpp where the implementation of SLTScript will live
    return false;
}

void Salty::AdvanceToNextRunnableStep(FrameContext* frame) {
    if (!frame) return;

    for (std::size_t i = frame->currentLine; i < frame->scriptTokens.size(); i++) {
        if (IsStepRunnable(frame->scriptTokens.at(i))) {
            frame->currentLine = i;
            return;
        }
    }

    frame->currentLine = frame->scriptTokens.size();
}