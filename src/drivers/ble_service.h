#pragma once
// BLE Service Handler - manages conversation state and message queueing

#include "Arduino.h"
#include "BLEd.h"
#include <queue>

struct QueuedMessage {
    String type;
    String role;
    String content;
    int chunk_index;
    int total_chunks;
};

class BLEService {
private:
    BLEService() : conversation_active(false), streaming_message(false) {}
    
    bool conversation_active;
    bool streaming_message;
    String current_message_id;
    String current_conversation_id;
    std::vector<BLEMessage> message_buffer;  // For reassembling chunks
    std::queue<String> pending_user_messages;  // Messages to send to LLM
    
public:
    static BLEService& get() {
        static BLEService instance;
        return instance;
    }
    
    void init() {
        auto& ble = BLEManager::get();
        
        // Register message RX callback
        ble.onMessageRX([this](BLEMessage msg) {
            handleMessageRX(msg);
        });
        
        // Register conversation control callback
        ble.onConversationControl([this](String action, String conv_id) {
            handleConversationControl(action, conv_id);
        });
        
        // Register terminal RX callback
        ble.onTerminalRX([this](String cmd) {
            handleTerminalRX(cmd);
        });
        
        Serial.println("[BLE Service] Initialized");
    }
    
    void handleMessageRX(BLEMessage msg) {
        if (debug) Serial.printf("[BLE] RX: type=%s role=%s chunk=%d/%d\n", 
            msg.type.c_str(), msg.role.c_str(), msg.chunk_index, msg.total_chunks);
        
        if (msg.type == "conversation_start") {
            current_conversation_id = msg.conversation_id;
            conversation_active = true;
            message_buffer.clear();
            Serial.printf("[BLE] Conversation started: %s\n", msg.conversation_id.c_str());
        }
        else if (msg.type == "conversation_clear") {
            message_buffer.clear();
            Serial.println("[BLE] Conversation cleared");
        }
        else if (msg.type == "message_chunk") {
            // Buffer chunk
            message_buffer.push_back(msg);
            streaming_message = true;
            current_message_id = msg.message_id;
            
            // If this is the last chunk, reassemble
            if (msg.chunk_index == msg.total_chunks - 1) {
                reassembleMessage();
            }
        }
        else if (msg.type == "message_complete") {
            // Complete message received
            if (msg.role == "user") {
                // User message from app - add to queue for LLM
                pending_user_messages.push(msg.content);
                if (debug) Serial.printf("[BLE] User message queued: %s\n", msg.content.c_str());
            }
            else if (msg.role == "assistant") {
                // Assistant response from app - display it
                if (debug) Serial.printf("[BLE] Assistant: %s\n", msg.content.c_str());
                // This will be rendered by LLM app
            }
        }
    }
    
    void handleConversationControl(String action, String conv_id) {
        if (debug) Serial.printf("[BLE] Conv Control: action=%s id=%s\n", action.c_str(), conv_id.c_str());
        
        if (action == "new") {
            current_conversation_id = conv_id.length() > 0 ? conv_id : String(millis());
            conversation_active = true;
            message_buffer.clear();
            pending_user_messages = std::queue<String>();  // Clear queue
        }
        else if (action == "switch") {
            current_conversation_id = conv_id;
            conversation_active = true;
            message_buffer.clear();
        }
        else if (action == "delete") {
            if (current_conversation_id == conv_id) {
                conversation_active = false;
                message_buffer.clear();
                current_conversation_id = "";
            }
        }
    }
    
    void handleTerminalRX(String cmd) {
        if (debug) Serial.printf("[BLE] Terminal RX: %s\n", cmd.c_str());
        
        // Execute terminal command and send response
        String response = process_command(cmd);
        BLEManager::get().sendTerminal(response + "\n");
    }
    
    void reassembleMessage() {
        // Sort chunks by index
        std::sort(message_buffer.begin(), message_buffer.end(), 
            [](const BLEMessage& a, const BLEMessage& b) {
                return a.chunk_index < b.chunk_index;
            });
        
        // Reassemble content
        String full_content = "";
        for (auto& msg : message_buffer) {
            full_content += msg.content;
        }
        
        if (debug) Serial.printf("[BLE] Reassembled message: %s\n", full_content.c_str());
        
        message_buffer.clear();
        streaming_message = false;
    }
    
    // Send user message to app
    void sendUserMessage(String content) {
        if (!BLEManager::get().connected()) {
            Serial.println("[BLE] Not connected, cannot send message");
            return;
        }
        
        BLEManager::get().sendMessage("user_message", content);
        if (debug) Serial.printf("[BLE] Sent user message: %s\n", content.c_str());
    }
    
    // Get next queued message for LLM (from app)
    bool getNextMessage(String& out_message) {
        if (pending_user_messages.empty()) return false;
        out_message = pending_user_messages.front();
        pending_user_messages.pop();
        return true;
    }
    
    bool hasConnection() {
        return BLEManager::get().connected();
    }
    
    bool isConversationActive() {
        return conversation_active && BLEManager::get().connected();
    }
    
    String getCurrentConversationId() {
        return current_conversation_id;
    }
};
