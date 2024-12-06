// Object Oriented Programming - Secret Message Encoder & Decoder
// Designed and Developed by Kobi Chambers - Griffith University

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

constexpr int global_max_size{static_cast<int>(std::sqrt(1000))};
constexpr int square(int x) { return x * x; }
constexpr int decoded_length(int x) { return (square(x) + 1) / 2; }

void clear_input_buffer() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string string_to_upper(std::string message) {
  for (auto &ch : message) {
    ch = (char)std::toupper(ch);
  }
  return message;
}

const char get_user_choice(const std::string message_to_user) {
  std::cout << message_to_user;
  char choice;
  while (!(std::cin >> choice) ||
         (choice != 'y' && choice != 'Y' && choice != 'n' && choice != 'N')) {
    std::cout << "Invalid input. Please enter (y/n): ";
    clear_input_buffer();
  }
  return (char)std::toupper(choice);
}

bool is_valid_utf8(char c) {
  return !(c >= 0xC0 && c <= 0xC1) && !(c >= 0xF5 && c <= 0xFF);
}

std::string sanitise_non_utf8(const std::string &input) {
  std::string sanitised;
  std::copy_if(input.begin(), input.end(), std::back_inserter(sanitised),
               is_valid_utf8);
  return sanitised;
}

class CustomException : public std::exception {
 private:
  std::string exception_message_;

 public:
  CustomException(const char *msg) : exception_message_{msg} {}

  const char *what() const noexcept override {
    return exception_message_.c_str();
  }
};

class GridBoundary {
 private:
  int top_, bottom_, right_;

 public:
  GridBoundary(int top, int bottom, int right)
      : top_{top}, bottom_{bottom}, right_{right} {}

  void manipulate_boundary() {
    ++top_;
    --bottom_;
    --right_;
  }

  bool within_row_bounds(int row) const {
    return (row > top_ && row < bottom_);
  }

  bool within_col_bounds(int col) const { return col < right_; }

  bool is_within_boundary(int row, int col) const {
    return (within_row_bounds(row) && within_col_bounds(col));
  }
};

class GridOperations {
 private:
  int grid_size_;
  std::unique_ptr<std::unique_ptr<char[]>[]> grid_;

  void initialise_grid(const std::string message) {
    int index{0};
    for (int i{0}; i < grid_size_; i++) {
      for (int j{0}; j < grid_size_; j++) {
        grid_[i][j] = (message != "") ? message[index++] : ' ';
      }
    }
  }

  void fill_grid(const std::string &message, const bool encode_flag,
                 std::string *decoded_message) {
    const int swap{-1};
    const int max_decoded_length{decoded_length(grid_size_)};

    GridBoundary boundary{0, grid_size_ - 1, grid_size_ - 1};
    int index{0};
    int row{grid_size_ / 2};
    int col{0};
    int row_manip{-1};
    int col_manip{1};
    while (index < message.length()) {
      if (encode_flag) {
        grid_[row][col] = message[index++];
      } else if (index >= max_decoded_length) {
        break;
      } else {
        *decoded_message += grid_[row][col];
        grid_[row][col] = '~';
        ++index;
      }
      if (!boundary.within_row_bounds(row)) {
        row_manip *= swap;
      }
      if (!boundary.within_col_bounds(col)) {
        col_manip *= swap;
      }

      row += row_manip;
      col += col_manip;
      if (((encode_flag && grid_[row][col] != ' ') ||
           (!encode_flag && grid_[row][col] == '~')) &&
          boundary.is_within_boundary(row, col)) {
        ++col;
        col_manip *= swap;
        boundary.manipulate_boundary();
      }
    }
  }

 public:
  GridOperations() : grid_size_{0}, grid_{nullptr} {}

  void set_grid_size(int size) {
    if (size < 3) {
      throw CustomException("\tMinimum grid size is 3x3.");
    }
    if (size % 2 == 0) {
      throw CustomException("\tGrid size must be an odd number.");
    }
    grid_size_ = size;
    grid_ = std::make_unique<std::unique_ptr<char[]>[]>(grid_size_);
    for (int i{0}; i < grid_size_; i++) {
      grid_[i] = std::make_unique<char[]>(grid_size_);
    }
  }

  void process_grid(const std::string &message, const bool encode_flag,
                    std::string *decoded_message) {
    encode_flag ? initialise_grid("") : initialise_grid(message);
    fill_grid(message, encode_flag, decoded_message);
  }

  std::string get_encoded_message() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution('A', 'Z');

    std::string encoded_message;
    for (int i{0}; i < grid_size_; i++) {
      for (int j{0}; j < grid_size_; j++) {
        encoded_message +=
            (grid_[i][j] == ' ') ? (char)(distribution(gen)) : grid_[i][j];
      }
    }
    return encoded_message;
  }
};

class EncoderDecoder {
 private:
  std::shared_ptr<GridOperations> grid_operations_;

  constexpr bool is_even(int x) { return x % 2 == 0; }

  int prompt_grid_size(int min_size) {
    if (get_user_choice("Declare custom grid size for encoding? (y/n): ") ==
        'N') {
      return min_size;
    }

    std::cout << "\tMinimum grid size for given message: " << min_size << '\n'
              << "Enter desired grid size for encoding: ";
    int new_size;
    while (!(std::cin >> new_size) || new_size < min_size ||
           new_size > global_max_size || is_even(new_size)) {
      if (new_size < min_size || new_size > global_max_size) {
        std::cout << "\tInvalid grid size - Min: " << min_size
                  << " Max: " << global_max_size << '\n';
      } else if (is_even(new_size)) {
        std::cout << "\tGrid size must be an odd number.\n";
      }
      std::cout << "Please enter a valid integer: ";
      clear_input_buffer();
    }
    return new_size;
  }

 public:
  EncoderDecoder() : grid_operations_{std::make_shared<GridOperations>()} {}

  std::string encode(const std::string &message,
                     bool is_auto_grid_size = false) {
    int min_size{static_cast<int>(std::ceil(std::sqrt(message.length())))};
    if (is_even(min_size)) {
      ++min_size;
    }
    int max_decoded_length{decoded_length(min_size)};
    while (message.length() > max_decoded_length) {
      min_size += 2;
      max_decoded_length = decoded_length(min_size);
    }
    if (square(min_size) > 999) {
      throw CustomException("\tEncoded message length must be <1000.");
    }
    if (is_auto_grid_size) {
      grid_operations_->set_grid_size(min_size);
    } else {
      grid_operations_->set_grid_size(prompt_grid_size(min_size));
      clear_input_buffer();
    }

    constexpr bool encode_flag{true};
    grid_operations_->process_grid(message, encode_flag, nullptr);
    return grid_operations_->get_encoded_message();
  }

  std::string decode(const std::string &encoded_message) {
    int grid_size{static_cast<int>(std::sqrt(encoded_message.length()))};
    if (is_even(grid_size)) {
      throw CustomException(
          "\tEncoded message length must be an odd square number.");
    }
    if (encoded_message.length() > 999) {
      throw CustomException("\tEncoded message length must be <1000.");
    }

    grid_operations_->set_grid_size(grid_size);
    constexpr bool encode_flag{false};
    std::string decoded_message;
    grid_operations_->process_grid(encoded_message, encode_flag,
                                   &decoded_message);
    return decoded_message;
  }
};

class FileOperations {
 private:
  std::unordered_set<std::string> get_directory_files() {
    std::unordered_set<std::string> cwd_files;
    for (const auto &entry : std::filesystem::directory_iterator(".")) {
      cwd_files.insert(entry.path().filename().string());
    }
    return cwd_files;
  }

  bool file_exists(const std::string &file_name,
                   const std::unordered_set<std::string> &cwd_files) {
    return cwd_files.find(file_name) != cwd_files.end();
  }

  std::string generate_default_file_name(int &default_iter) {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << default_iter++;
    return "default_" + ss.str() + ".txt";
  }

  std::string prompt_file_name(const std::unordered_set<std::string> cwd_files,
                               const bool is_new_file) {
    std::string file_name;
    std::cout
        << "Enter the filename (leave empty for default, 'menu' to return): ";
    std::getline(std::cin, file_name);

    int default_iter{0};
    if (file_name.empty()) {
      std::cout << "\tSearching for default filenames..." << '\n';
      file_name = "default_00.txt";
      ++default_iter;
    } else if (string_to_upper(file_name) == "MENU") {
      throw CustomException("\tReturning to main menu...");
    }

    while ((default_iter > 0 && is_new_file &&
            file_exists(file_name, cwd_files) && default_iter < 100) ||
           (default_iter > 0 && !is_new_file &&
            !file_exists(file_name, cwd_files) && default_iter < 100)) {
      file_name = generate_default_file_name(default_iter);
    }
    return file_name;
  }

  std::string get_new_file_name() {
    const std::unordered_set<std::string> cwd_files{get_directory_files()};
    constexpr bool is_new_file{true};
    clear_input_buffer();
    while (true) {
      std::string file_name{prompt_file_name(cwd_files, is_new_file)};
      if (!file_exists(file_name, cwd_files)) {
        return file_name;
      }
      if (get_user_choice(
              "File already exists. Do you want to overwrite? (y/n): ") ==
          'Y') {
        return file_name;
      }
      std::cout << "Enter a new filename.\n";
      clear_input_buffer();
    }
  }

  std::string get_existing_file_name() {
    std::cout << "Contents of current directory:\n";
    const std::unordered_set<std::string> cwd_files{get_directory_files()};
    const std::string program_name{
        std::filesystem::path(__FILE__).filename().string()};
    const std::string executable_name{
        std::filesystem::path(__argv[0]).filename().string()};
    for (auto file : cwd_files) {
      if (file != program_name && file != executable_name) {
        std::cout << file << '\n';
      }
    }
    std::cout << '\n';
    clear_input_buffer();

    constexpr bool is_new_file{false};
    std::string file_name;
    while (true) {
      file_name = prompt_file_name(cwd_files, is_new_file);
      if (file_exists(file_name, cwd_files)) {
        std::cout << "\tLoading messages from filename '" << file_name
                  << "'...\n";
        return file_name;
      }
      std::cout << "\tFile does not exist.\n";
    }
  }

 public:
  std::vector<std::string> load_from_file() {
    const std::string file_name{get_existing_file_name()};
    std::ifstream file(file_name);
    if (!file.is_open()) {
      throw CustomException("\tError opening file. Ensure correct file type.");
    }

    std::vector<std::string> messages;
    std::string message;
    while (std::getline(file, message)) {
      messages.push_back(string_to_upper(sanitise_non_utf8(message)));
    }
    file.close();
    if (messages.empty()) {
      throw CustomException("\tNo messages found in the file.");
    }
    return messages;
  }

  void save_to_file(const std::vector<std::string> &messages) {
    if (messages.empty()) {
      throw CustomException("\tNo messages to save.");
    }

    const std::string file_name{get_new_file_name()};
    std::ofstream file(file_name);
    if (!file.is_open()) {
      throw CustomException("\tError opening file.");
    }

    for (auto &message : messages) {
      file << message << '\n';
    }
    std::cout << "\tMessages saved to file '" << file_name << "'" << std::endl;
    file.close();
  }
};

enum class MessageType { raw, encoded, decoded };

class MessageBuffer {
 private:
  std::string raw_message_;
  std::string encoded_message_;
  std::string decoded_message_;

 public:
  MessageBuffer() {}

  void set_message(const std::string message, const MessageType message_type) {
    if (message.empty()) {
      throw CustomException("\tNo message entered.\n");
    }

    switch (message_type) {
      case MessageType::raw: {
        encoded_message_.clear();
        decoded_message_.clear();
        std::cout << "\tMessage buffers cleared..." << '\n';
        raw_message_ = message;
        std::cout << "\tStored message: " << raw_message_ << std::endl;
        break;
      }
      case MessageType::encoded: {
        if (!raw_message_.empty()) {
          decoded_message_ = raw_message_;
          raw_message_.clear();
        }
        encoded_message_ = message;
        std::cout << "\tStored encoded message: " << encoded_message_
                  << std::endl;
        break;
      }
      case MessageType::decoded: {
        if (!raw_message_.empty()) {
          encoded_message_ = raw_message_;
          raw_message_.clear();
        }
        decoded_message_ = message;
        std::cout << "\tStored decoded message: " << decoded_message_
                  << std::endl;
        break;
      }
      default: {
        throw CustomException("\tInvalid message type.\n");
      }
    }
  }

  std::string get_message(MessageType message_type) {
    switch (message_type) {
      case MessageType::raw: {
        return raw_message_;
      }
      case MessageType::encoded: {
        return encoded_message_;
      }
      case MessageType::decoded: {
        return decoded_message_;
      }
      default: {
        throw CustomException("\tInvalid message type.\n");
      }
    }
  }

  bool is_empty() const {
    return encoded_message_.empty() && decoded_message_.empty();
  }

  void clear_message_buffer() {
    raw_message_.clear();
    encoded_message_.clear();
    decoded_message_.clear();
    std::cout << "\n\tAll stored messages have been wiped." << std::endl;
  }
};

class Driver {
 private:
  std::shared_ptr<FileOperations> file_operations_;
  std::shared_ptr<EncoderDecoder> encoder_decoder_;
  std::shared_ptr<MessageBuffer> message_buffer_;

  std::string get_input_message() {
    clear_input_buffer();
    std::cout << "\nWARNING: This will clear all message buffers, leave blank "
                 "to return.\n"
              << "Enter the message: ";
    std::string message;
    std::getline(std::cin, message);
    if (message.empty()) {
      throw CustomException("\tNo message entered by user.");
    }
    if (message.length() < 2) {
      throw CustomException("\tMessage must be more than one character.");
    }
    return string_to_upper(message);
  }

  void display_messages(const std::vector<std::string> &messages) {
    int message_count{1};
    for (const auto &message : messages) {
      std::cout << "\tMessage " << message_count++ << ": " << message << '\n';
    }
  }

  void process_message_selection(const std::vector<std::string> &messages) {
    std::cout << "\nWARNING: This will clear all message buffers (enter 0 to "
                 "return, -1 encode all, -2 decode all).\n"
              << "Select desired message to save to buffer: ";
    int message_selection;
    while (!(std::cin >> message_selection) || message_selection < 0 ||
           message_selection > messages.size()) {
      switch (message_selection) {
        case (0): {
          throw CustomException("\tReturning to main menu...");
        }
        case (-1): {
          encode_all_messages(messages);
          return;
        }
        case (-2): {
          decode_all_messages(messages);
          return;
        }
        default: {
          std::cout << "\tInvalid input (enter 0 to return)...\n"
                    << "Select a message between 1 and " << messages.size()
                    << ": ";
          clear_input_buffer();
        }
      }
    }
    // Valid message selection
    message_buffer_->set_message(messages[message_selection - 1],
                                 MessageType::raw);
  }

  void encode_all_messages(const std::vector<std::string> &messages) {
    std::cout << "Encoding all messages to new file...\n";
    constexpr bool is_auto_grid_size{true};
    std::vector<std::string> encoded_messages;
    for (const auto &message : messages) {
      if (message.empty()) {
        encoded_messages.push_back("");
      } else {
        // Try to add message to vector, catch exceptions to prevent further
        // stack unwinding
        try {
          encoded_messages.push_back(
              encoder_decoder_->encode(message, is_auto_grid_size));
        } catch (const CustomException &) {
          // Add line for failures - we use 'FE:: / FD::' as it is ambiguous to
          // users unfamiliar with the encryption
          encoded_messages.push_back("FE::" + message);
        }
      }
    }
    process_messages(encoded_messages);
  }

  void decode_all_messages(const std::vector<std::string> &messages) {
    std::vector<std::string> decoded_messages;
    std::cout << "Decoding all messages to new file.\n";
    for (const auto &message : messages) {
      if (message.empty()) {
        decoded_messages.push_back("");
      } else {
        try {
          decoded_messages.push_back(encoder_decoder_->decode(message));
        } catch (const CustomException &) {
          decoded_messages.push_back("FD::" + message);
        }
      }
    }
    process_messages(decoded_messages);
  }

  void process_messages(const std::vector<std::string> &messages) {
    messages.empty()
        ? throw CustomException("Failed to process... Check contents of file.")
        : file_operations_->save_to_file(messages);
  }

 public:
  Driver()
      : file_operations_{std::make_shared<FileOperations>()},
        encoder_decoder_{std::make_shared<EncoderDecoder>()},
        message_buffer_{std::make_shared<MessageBuffer>()} {}

  void get_message_from_user() {
    message_buffer_->set_message(get_input_message(), MessageType::raw);
  }

  void get_messages_from_file() {
    const std::vector<std::string> messages{file_operations_->load_from_file()};
    display_messages(messages);
    process_message_selection(messages);
  }

  void encode_user_message() {
    if (!message_buffer_->get_message(MessageType::decoded).empty()) {
      std::cout << "\tFound decoded message in buffer: "
                << message_buffer_->get_message(MessageType::decoded) << '\n';
      if (get_user_choice("Would you like to encode this? (y/n): ") == 'Y') {
        message_buffer_->set_message(
            encoder_decoder_->encode(
                message_buffer_->get_message(MessageType::decoded)),
            MessageType::encoded);
        return;
      }
    }

    if (message_buffer_->get_message(MessageType::raw).empty()) {
      std::cout
          << "\tNo raw messages in buffer. Getting new message from user..."
          << std::endl;
      get_message_from_user();
    }
    message_buffer_->set_message(
        encoder_decoder_->encode(
            message_buffer_->get_message(MessageType::raw)),
        MessageType::encoded);
  }

  void decode_user_message() {
    if (!message_buffer_->get_message(MessageType::encoded).empty()) {
      std::cout << "\tFound encoded message in buffer: "
                << message_buffer_->get_message(MessageType::encoded) << '\n';
      if (get_user_choice("Would you like to decode this? (y/n): ") == 'Y') {
        message_buffer_->set_message(
            encoder_decoder_->decode(
                message_buffer_->get_message(MessageType::encoded)),
            MessageType::decoded);
        return;
      }
    }

    if (message_buffer_->get_message(MessageType::raw).empty()) {
      std::cout
          << "\tNo raw messages in buffer. Getting new message from user..."
          << std::endl;
      get_message_from_user();
    }
    message_buffer_->set_message(
        encoder_decoder_->decode(
            message_buffer_->get_message(MessageType::raw)),
        MessageType::decoded);
  }

  void save_messages_to_file() {
    if (message_buffer_->is_empty()) {
      throw CustomException(
          "Buffer is empty. No encoded / decoded messages to save.");
    }
    file_operations_->save_to_file(
        {message_buffer_->get_message(MessageType::encoded),
         message_buffer_->get_message(MessageType::decoded)});
    message_buffer_->clear_message_buffer();
  }
};

class UserInterface {
 private:
  int get_menu_option() {
    std::cout << '\n'
              << "*****************************************************\n"
              << "* 1, Enter a message                                *\n"
              << "* 2, Load a message from a file                     *\n"
              << "* 3, Encode a message                               *\n"
              << "* 4, Decode a message                               *\n"
              << "* 5, Save the message & decoded message to a file.  *\n"
              << "* 6, Quit                                           *\n"
              << "*****************************************************\n"
              << "Select option: ";
    int menu_option;
    while (!(std::cin >> menu_option) || menu_option < 1 || menu_option > 6) {
      std::cout
          << "Invalid input. Please enter a menu option between 1 and 6: ";
      clear_input_buffer();
    }
    return menu_option;
  }

 public:
  void run_coder() {
    std::unique_ptr<Driver> driver_{std::make_unique<Driver>()};

    constexpr int num_options{6};
    using OptionFunction = void (Driver::*)();
    OptionFunction options[num_options] = {
        &Driver::get_message_from_user, &Driver::get_messages_from_file,
        &Driver::encode_user_message, &Driver::decode_user_message,
        &Driver::save_messages_to_file};

    // Loop until user enters option '6'
    int option{get_menu_option()};
    while (option != num_options) {
      try {
        if (option > 0 && option <= num_options) {
          (driver_.get()->*options[option - 1])();
        } else {
          std::cerr << "\tInvalid option.\n";
        }
      } catch (const CustomException &e) {
        std::cerr << e.what() << '\n';
      }
      option = get_menu_option();
    }
    std::cout << "\tExiting program..." << std::endl;
  }
};

int main() {
  std::unique_ptr<UserInterface> user_interface{
      std::make_unique<UserInterface>()};
  user_interface->run_coder();
  return 0;
}