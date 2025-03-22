#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <Arduino.h>

class RateLimiter {
private:
    // Configurable rate limit parameters
    static const int MAX_ATTEMPTS;
    static const unsigned long BLOCK_DURATION;
    
    // Structure to track login attempts
    struct AttemptRecord {
        int attempts;
        unsigned long lastAttemptTime;
        unsigned long blockUntil;
    };

    // Current attempt record
    AttemptRecord loginAttempts;

public:
    // Constructor to initialize default state
    RateLimiter();

    // Check if login attempt is allowed
    bool canAttemptLogin();

    // Record a login attempt
    void recordAttempt();

    // Reset attempt tracking (successful login)
    void resetAttempts();

    // Get remaining time until next login attempt is possible
    unsigned long getRemainingBlockTime();
};

#endif // RATE_LIMITER_H