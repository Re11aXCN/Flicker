#ifndef FILE_READ_H_
#define FILE_READ_H_

#include <print>
#include <fstream>
#include <filesystem>
#include <expected>

// Error code enum
enum class FileErrorCode
{
    FileNotFound,
    PermissionDenied,
    NotAFile,
    ReadFailure,
    FileTooLarge,
    Unknown
};

// Structured error with context
struct FileError
{
    FileErrorCode code;
    std::filesystem::path path;
    std::string message;

    friend std::ostream& operator<<(std::ostream& os, FileError error)
    {
        os << "FileError[ " << error.path << "]: ";
        switch (error.code)
        {
        case FileErrorCode::FileNotFound:       os << "File not found."; break;
        case FileErrorCode::PermissionDenied:   os << "Permission denied."; break;
        case FileErrorCode::NotAFile:           os << "Path is not a regular file."; break;
        case FileErrorCode::ReadFailure:        os << "Failed to read file."; break;
        case FileErrorCode::FileTooLarge:       os << "File exceeds size limits."; break;
        case FileErrorCode::Unknown:            os << "Unknown error."; break;
        }
        if (!error.message.empty())
            os << " ( " << error.message << " )";
        return os;
    }
};

constexpr std::uintmax_t kMaxAssetFileSize = 1024 * 1024 * 100; // 100MB

std::expected<std::vector<char>, FileError> ReadFile(const std::filesystem::path& path)
{
    std::error_code ec;

    // 1. Existence check
    if (!std::filesystem::exists(path, ec))
        return std::unexpected(FileError{ FileErrorCode::FileNotFound, path, ec.message() });

    // 2. Type check
    if (!std::filesystem::is_regular_file(path, ec))
        return std::unexpected(FileError{ FileErrorCode::NotAFile, path, ec.message() });

    // 3. Size check
    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec)
        return std::unexpected(FileError{ FileErrorCode::ReadFailure, path, ec.message() });

    if (fileSize > kMaxAssetFileSize)
        return std::unexpected(FileError{ FileErrorCode::FileTooLarge, path, "Too big for asset loader." });

    // 4. Open and read
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return std::unexpected(FileError{ FileErrorCode::PermissionDenied, path, "Failed to open file." });

    std::vector<char> buffer(fileSize);
    if(!file.read(buffer.data(), fileSize))
        return std::unexpected(FileError{ FileErrorCode::ReadFailure, path, "Stream read failure." });

    return buffer;
}

void example() 
{
    std::filesystem::path path = "C:/Users/User/Desktop/example.txt";
    {
        auto result = ReadFile(path);
        if (result)
        {
            std::println("Loaded file: {} successfully, size: {}", path.string(), result->size());
        }
        else
        {
            const auto& error = result.error();
            std::println("Failed to load file: {}, code: {}, message: {}", error.path, error.code, error.message);
        }
    }
    {
        auto result = ReadFile(path)
            .transform([](const auto& buffer) 
            {
                // data -> assert loading system
                return buffer; // expected object.first, not expected object.second
            })
            .or_else([](const auto& error)
            {
                // error handling
            });
    }
    // and_then the difference between transform,
    // 
    // and_then chained called expected objects
    // transform only called expected objects.first like rust unwrap ?
    {
        // Chained call delivery error
        auto result = ReadFile(path)
            .and_then([&](const auto& buffer) // maybe success
            {
                // data -> assert loading system
                return ReadFile(path); // expected object
            })
            .and_then([&](const auto& buffer) // error
            {
                // data -> assert loading system
                return ReadFile(path);
            })
            .and_then([&](const auto& buffer) // error
            {
                // data -> assert loading system
                return ReadFile(path);
            })
            .or_else([](const auto& error)
            {
                // error handling
            });
    }
}
#endif // !FILE_READ_H_