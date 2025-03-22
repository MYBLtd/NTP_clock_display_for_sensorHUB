#include "auth_manager.h"
#include "CustomHash.h"
#include <Preferences.h>

// Define static class members
const char* AuthenticationManager::PREF_NAMESPACE = "auth";
const uint8_t AuthenticationManager::MIN_USERNAME_LENGTH;  // Value already defined in header
const uint8_t AuthenticationManager::MIN_PASSWORD_LENGTH;  // Value already defined in header

bool AuthenticationManager::validateCredentials(const String& username, const String& password) {
    // Early validation for empty credentials
    if (username.isEmpty() || password.isEmpty()) {
        return false;
    }
    
    // Get stored values from preferences
    Preferences preferences;
    preferences.begin(PREF_NAMESPACE, true);  // Read-only mode
    
    String storedUsername = preferences.getString("username", "");
    String storedHash = preferences.getString("password_hash", "");
    
    preferences.end();
    
    // Verify username first
    if (storedUsername != username) {
        return false;
    }
    
    // If we have no stored hash, authentication fails
    if (storedHash.isEmpty()) {
        return false;
    }
    
    // Calculate hash of provided password and compare
    String calculatedHash = CustomHash::sha256(password);
    return calculatedHash.equals(storedHash);
}

bool AuthenticationManager::setCredentials(const String& username, const String& password) {
    // Validate input lengths
    if (username.length() < MIN_USERNAME_LENGTH || password.length() < MIN_PASSWORD_LENGTH) {
        return false;
    }
    
    // Generate password hash
    String passwordHash = CustomHash::sha256(password);
    
    // Store credentials
    Preferences preferences;
    preferences.begin(PREF_NAMESPACE, false);  // Write mode
    
    bool success = preferences.putString("username", username) && 
                  preferences.putString("password_hash", passwordHash);
    
    preferences.end();
    return success;
}

String AuthenticationManager::getStoredUsername() {
    Preferences preferences;
    preferences.begin(PREF_NAMESPACE, true);  // Read-only mode
    String username = preferences.getString("username", "");
    preferences.end();
    
    return username;
}

// Constructor implementation
AuthenticationManager::AuthenticationManager() : 
    currentToken(""),
    tokenCreationTime(0) {
    // Initialization code if needed
}

String AuthenticationManager::generateToken() {
    // Simple token generation
    String token = String(random(0xFFFFFFFF), HEX);
    currentToken = token;
    tokenCreationTime = millis();
    return token;
}

bool AuthenticationManager::validateToken(const String& providedToken) {
    if (currentToken.isEmpty() || providedToken.isEmpty()) {
        return false;
    }
    
    // Check token expiry
    if (millis() - tokenCreationTime > TOKEN_EXPIRY) {
        invalidateToken();
        return false;
    }
    
    return currentToken.equals(providedToken);
}

void AuthenticationManager::invalidateToken() {
    currentToken = "";
    tokenCreationTime = 0;
}