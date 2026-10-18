// Utils.h
#pragma once
#include <string>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX    
#include <windows.h>
#endif

// 実行ファイルのあるディレクトリを返す
inline std::string getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path().string();
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    buf[len] = '\0';
    return std::filesystem::path(buf).parent_path().string();
#endif
}

// 実行ファイル基準の相対パスを絶対パスに変換する
inline std::string resolvePath(const std::string& relative) {
    return (std::filesystem::path(getExeDir()) / relative).string();
}