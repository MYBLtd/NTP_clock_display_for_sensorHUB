#include "rate_limiter.h"

// Define static constants
const int RateLimiter::MAX_ATTEMPTS = 5;
const unsigned long RateLimiter::BLOCK_DURATION = 15 * 60 * 1000;  // 15 minutes

RateLimiter::RateLimiter() {
    // Initialize all tracking values to zero
    loginAttempts = {0, 0, 0};
}

bool RateLimiter::canAttemptLogin() {
    unsigned long now = millis();
    
    // Check if still blocked from previous attempts
    if (now < loginAttempts.blockUntil) {
        return false;
    }
    
    // Reset block if enough time has passed
    if (now - loginAttempts.lastAttemptTime > BLOCK_DURATION) {
        loginAttempts.attempts = 0;
    }
    
    // Allow login if max attempts not reached
    return loginAttempts.attempts < MAX_ATTEMPTS;
}

void RateLimiter::recordAttempt() {
    unsigned long now = millis();
    
    loginAttempts.attempts++;
    loginAttempts.lastAttemptTime = now;
    
    // If max attempts reached, block further attempts
    if (loginAttempts.attempts >= MAX_ATTEMPTS) {
        loginAttempts.blockUntil = now + BLOCK_DURATION;
    }
}

void RateLimiter::resetAttempts() {
    loginAttempts.attempts = 0;
    loginAttempts.blockUntil = 0;
}

unsigned long RateLimiter::getRemainingBlockTime() {
    unsigned long now = millis();
    
    // If not blocked, return 0
    if (now >= loginAttempts.blockUntil) {
        return 0;
    }
    
    // Return remaining block time
    return loginAttempts.blockUntil - now;
}