/**
 * @file error-codes.c
 * @brief Реализация утилит для работы с ошибками
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "error-codes.h"
#include <stdio.h>

const char* mt_error_string(int error_code) {
    switch (error_code) {
        case MT_SUCCESS:
            return "Success";
        
        /* General errors */
        case MT_ERR_GENERAL:
            return "General error";
        case MT_ERR_NO_MEMORY:
            return "Out of memory";
        case MT_ERR_INVALID_ARGS:
            return "Invalid arguments";
        case MT_ERR_INIT:
            return "Initialization error";
        case MT_ERR_INVALID_STATE:
            return "Invalid state";
        case MT_ERR_OVERFLOW:
            return "Overflow";
        case MT_ERR_NOT_SUPPORTED:
            return "Not supported";
        case MT_ERR_TIMEOUT:
            return "Timeout";
        
        /* Network errors */
        case MT_ERR_SOCKET:
            return "Socket error";
        case MT_ERR_CONNECT:
            return "Connection error";
        case MT_ERR_BIND:
            return "Bind error";
        case MT_ERR_LISTEN:
            return "Listen error";
        case MT_ERR_ACCEPT:
            return "Accept error";
        case MT_ERR_SEND:
            return "Send error";
        case MT_ERR_RECV:
            return "Receive error";
        case MT_ERR_DNS:
            return "DNS resolution error";
        case MT_ERR_SSL:
            return "SSL/TLS error";
        
        /* Fragmentation errors */
        case MT_ERR_FRAGMENT:
            return "Fragmentation error";
        case MT_ERR_ASSEMBLE:
            return "Assembly error";
        case MT_ERR_TOO_MANY_FRAGMENTS:
            return "Too many fragments";
        case MT_ERR_FRAGMENT_SIZE:
            return "Invalid fragment size";
        
        /* Padding errors */
        case MT_ERR_PADDING:
            return "Padding error";
        case MT_ERR_BLOCK_SIZE:
            return "Invalid block size";
        case MT_ERR_LENGTH_PREFIX:
            return "Length prefix error";
        
        /* ML errors */
        case MT_ERR_ML_MODEL:
            return "ML model error";
        case MT_ERR_ML_TRAIN:
            return "ML training error";
        case MT_ERR_ML_PREDICT:
            return "ML prediction error";
        case MT_ERR_ML_INSUFFICIENT_DATA:
            return "Insufficient data for ML";
        case MT_ERR_ML_FEATURES:
            return "ML features error";
        
        /* Performance errors */
        case MT_ERR_PERF_MONITOR:
            return "Performance monitor error";
        case MT_ERR_METRICS:
            return "Metrics collection error";
        case MT_ERR_THRESHOLD:
            return "Threshold exceeded";
        
        /* CLI errors */
        case MT_ERR_CLI:
            return "CLI error";
        case MT_ERR_CLI_AUTH:
            return "CLI authentication error";
        case MT_ERR_CLI_COMMAND:
            return "CLI command error";
        
        default:
            return "Unknown error";
    }
}

int mt_error_category(int error_code) {
    if (error_code == MT_SUCCESS) {
        return 0;  /* Success */
    }
    
    if (error_code >= -1 && error_code <= -99) {
        return 1;  /* General errors */
    }
    
    if (error_code >= -100 && error_code <= -199) {
        return 2;  /* Network errors */
    }
    
    if (error_code >= -200 && error_code <= -299) {
        return 3;  /* Fragmentation errors */
    }
    
    if (error_code >= -300 && error_code <= -399) {
        return 4;  /* Padding errors */
    }
    
    if (error_code >= -400 && error_code <= -499) {
        return 5;  /* ML errors */
    }
    
    if (error_code >= -500 && error_code <= -599) {
        return 6;  /* Performance errors */
    }
    
    if (error_code >= -600 && error_code <= -699) {
        return 7;  /* CLI errors */
    }
    
    return 99;  /* Unknown category */
}
