#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <Arduino.h>
#include <base64.h>

class AuthenticationManager {
public:
    // Constructor
    AuthenticationManager();

    // Token generation methods
    String generateToken();
    String generateSecureToken(bool useEncryption = false);

    // Token validation methods
    bool validateToken(const String& providedToken);
    void invalidateToken();

    // Credential management methods
    static bool validateCredentials(const String& username, const String& password);
    static bool setCredentials(const String& username, const String& password);
    static String getStoredUsername();

private:
    // Token configuration constants
    static const unsigned long TOKEN_EXPIRY = 3600000;  // 1 hour in milliseconds
    
    // Internal token tracking
    String currentToken;
    unsigned long tokenCreationTime;
    
    // Internal encryption methods
    String encryptToken(const String& token);
    String decryptToken(const String& encryptedToken);

    // Constants for validation
    static const uint8_t MIN_USERNAME_LENGTH = 3;
    static const uint8_t MIN_PASSWORD_LENGTH = 8;
    
    // Namespace for preferences storage
    static const char* PREF_NAMESPACE;
};

#endif // AUTH_MANAGER_H