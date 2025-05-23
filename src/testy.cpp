#pragma once
#include <atomic>
#include <thread>
#include <chrono>
#include "testy.h"

GoldGiver::GoldGiver() : running(false) {}

GoldGiver::~GoldGiver() {
    Stop();
}

GoldGiver& GoldGiver::GetSingleton() {
    static GoldGiver instance;
    return instance;
}

void GoldGiver::Start() {
    if (running) return;
    running = true;
    try {
        worker = std::thread([this]() { this->ThreadMain(); });
    } catch (const std::system_error& e) {
    }
}

void GoldGiver::Stop() {
    running = false;
    if (worker.joinable()) {
        worker.join();
    } else {
    }
}

void GoldGiver::ThreadMain() {
    while (running) {
        static int loopcounter = 0;
        SKSE::GetTaskInterface()->AddTask([]() {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player) {
                constexpr int kGoldFormID = 0x0000000F;
                auto gold = RE::TESForm::LookupByID<RE::TESObjectMISC>(kGoldFormID);
                if (gold) {
                    player->AddObjectToContainer(gold, nullptr, 1, nullptr);
                }
            }
        });
        int sleepseconds = 1;
        std::this_thread::sleep_for(std::chrono::seconds(sleepseconds));
    }
}
