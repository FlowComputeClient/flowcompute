#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>

#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <memory>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <zlib.h>

#include <nlohmann/json.hpp>

#include "render_data.h"

#define SERVER_VERSION "1.0.0"

using asio::ip::tcp;
using json = nlohmann::json;
namespace fs = std::filesystem;

enum class FileRequestType {
    Content,
    Stats,
    ContentAndStats
};

class WslSession : public std::enable_shared_from_this<WslSession> {
public:
    WslSession(tcp::socket socket) : m_socket(std::move(socket)) {}
    void start() { do_read(); }

private:
    std::queue<std::string> m_write_queue;
    std::string m_outgoing_json, m_outgoing_file_buffer;
    std::vector<asio::const_buffer> m_outgoing_buffers;

    void do_read() {
        auto self(shared_from_this());
        asio::async_read_until(m_socket, asio::dynamic_buffer(m_data), '\n',
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string request_str = m_data.substr(0, length);
                    m_data.erase(0, length);
                    process_request(request_str);
                    do_read();
                }
            });
    }

    void process_request(const std::string& request_str) {
        try {
            json request;
            try {
                request = json::parse(request_str);
            } catch (const json::exception& e) {
                std::cerr << "JSON error: " << e.what() << '\n';
                do_write_string("{\"status\":\"error\",\"message\":\"Invalid JSON\"}\n");
                return;
            }
            std::string action = request.value("action", "");
			if (action == "getVersion") {
				json response = handle_get_version();
                do_write_string(response.dump() + "\n");
			}
			else if (action == "shutdown") {
				json response = handle_shutdown();
                do_write_string(response.dump() + "\n");				
			}			
            else if (action == "checkOpenFoam") {
                json response = handle_check_openfoam();
                do_write_string(response.dump() + "\n");
            }
            else if (action == "launchShortUtility") {
                json response = handle_launch_short_utility(request.value("message", ""));
                do_write_string(response.dump() + "\n");
            }
            else if (action == "launchLongUtility") {
                handle_launch_long_utility(request.value("message", ""));
            }
            else if (action == "getTimesAndFields") {
                json response = handle_get_times_fields(request.value("message", ""));
                do_write_string(response.dump() + "\n");
            }
            else if (action == "writeData") {
                std::string path = request.value("message", "");
                size_t byteSize = request.value("byteSize", 0);
                json response = handle_write_data(path, byteSize);
                do_write_string(response.dump() + "\n");
            }
            else if (action == "findTutorials") {
                json response = handle_find_tutorials(request.value("message", ""));
                do_write_string(response.dump() + "\n");
            }
            else if (action == "copyTutorialFolders") {
                std::string msg = request.value("message", "");
                size_t pos = msg.find(',');
                if (pos != std::string::npos) {
                    std::string str1 = std::string(msg.substr(0, pos));
                    std::string str2 = std::string(msg.substr(pos + 1));
                    json response = handle_copy_tutorials(str1, str2);
                    do_write_string(response.dump() + "\n");
                }
            }
            else if (action == "getFileContent") {
                int requestType = request.value("requestType", 0);
                handle_get_file_content(request.value("message", ""), static_cast<FileRequestType>(requestType));
            }
            else if (action == "processPaths") {
                json response = handle_process_paths(request.value("message", ""), request.value("opType", 0));
                do_write_string(response.dump() + "\n");
            }
			else if (action == "getResult") {
                handle_get_result(request.value("message", ""));
			}			
            else {
                do_write_string("{\"status\":\"error\",\"message\":\"Unknown action\"}\n");
            }
        } catch (const std::exception& e) {
            do_write_string("{\"status\":\"error\",\"message\":\"Execution error\"}\n");
        }
    }

	// Check version
	json handle_get_version() {
        json result;
        result["status"] = "success";
        result["message"] = SERVER_VERSION;
        return result;
    }

    // Shut down server
    json handle_shutdown() {
        json result;
        result["status"] = "success";
        result["message"] = "WSL server shutting down.";

        // Wait and then shut down
        auto timer = std::make_shared<asio::steady_timer>(m_socket.get_executor());
        timer->expires_after(std::chrono::milliseconds(100));
        timer->async_wait([](const std::error_code&) {
            std::exit(0); 
        });
        return result;
    }

	void handle_get_result(const std::string& message) {	
		// Parse the pipe-separated request string
		std::vector<std::string> patches;
		std::stringstream ss(message);
		std::string token;
		json response;	
		
		while (std::getline(ss, token, '|')) {
			if (!token.empty()) {
				patches.push_back(token);
			}
		}

		// Validate the request format
		if (patches.size() < 2) {
			json response;
			response["status"] = "error";
			response["message"] = "Invalid request format. Expected: field_path|patch1|...";
			do_write_string(response.dump() + "\n");
			return;
		}

		// Get field path
		std::string field_path = patches[0];		
		if (!fs::exists(field_path)) {
			response["status"] = "error";
			response["message"] = "Field path not found: " + field_path;
			do_write_string(response.dump() + "\n");
			return;
		}	
		
		// Get field data
		patches.erase(patches.begin());
		
		std::cout << "field_path = " << field_path << std::endl;
		std::cout << "patches[0] = " << patches[0] << std::endl;
		
		std::vector<FieldData> fieldData = read_openfoam_field(field_path, patches);
		
		// Transfer field data
		auto payload = std::make_shared<FieldDataPayload>();
		payload->fields = std::move(fieldData);

		// Calculate byte sizes and populate the fieldSizes array
		uint32_t totalDataBytes = 0;
		for (const auto& field : payload->fields) {
			payload->fieldSizes.push_back(field.numElements);
			totalDataBytes += field.numElements * sizeof(float);
		}

		// Configure the header
		payload->header.magicNumber = 0xFEEDBEEF;
		payload->header.numFields = static_cast<uint32_t>(payload->fields.size());
		payload->header.sizesByteSize = static_cast<uint32_t>(payload->fieldSizes.size() * sizeof(uint32_t));
		payload->header.dataByteSize = totalDataBytes;

		// Populate JSON
		response["status"] = "success";
		response["type"] = "field_result";
		payload->jsonString = response.dump() + "\n";

		// Queue buffers
		m_outgoing_buffers.clear();
		m_outgoing_buffers.push_back(asio::buffer(payload->jsonString));
		m_outgoing_buffers.push_back(asio::buffer(&payload->header, sizeof(FieldDataHeader)));

		// Queue the sizes array block
		if (!payload->fieldSizes.empty()) {
			m_outgoing_buffers.push_back(asio::buffer(payload->fieldSizes));
		}

		// Queue each float vector directly from the moved FieldData structures
		for (const auto& field : payload->fields) {
			if (!field.elementVals.empty()) {
				m_outgoing_buffers.push_back(asio::buffer(field.elementVals));
			}
		}

		// Transmit asynchronously
		auto self(shared_from_this());
		m_socket.set_option(asio::ip::tcp::no_delay(true));
		asio::async_write(m_socket, m_outgoing_buffers,
			[this, self, payload](std::error_code ec, std::size_t) {
				if (ec) {
					std::cerr << "Field data transfer error: " << ec.message() << "\n";
				}
			});
		m_socket.set_option(asio::ip::tcp::no_delay(false));
	}

	json handle_process_paths(const std::string& paths_string, int op_type) {
		json response;
		json message_array = json::array();
		bool global_success = true;

		// Added RENAME and COPY to the enum
		enum class PathOp { CREATE = 0, DELETE, CHECK, LIST, RENAME, COPY };
		PathOp op = static_cast<PathOp>(op_type);

        // Handle LIST with an empty input string
        if (op == PathOp::LIST && paths_string.empty()) {
            const char* home_env = std::getenv("HOME");
            std::string home_dir = home_env ? home_env : "/";
            
            message_array.push_back(home_dir);
            try {
                auto options = std::filesystem::directory_options::skip_permission_denied;
                for (const auto& entry : std::filesystem::directory_iterator(home_dir, options)) {
                    std::string item_name = entry.path().filename().string();
                    if (!item_name.empty() && item_name.front() == '.') continue;
                    if (entry.is_regular_file()) item_name += "|";
                    message_array.push_back(item_name);
                }
            } catch (...) {
                global_success = false;
            }
            
            response["status"] = global_success ? "success" : "failure";
            response["message"] = message_array;
            return response;
        }

		// Error handling if input is empty for other operations
		if (paths_string.empty()) {
			response["status"] = "failure";
			response["message"] = json::array({"Input paths string was empty."});
			return response;
		}

		// Process the delimited string
		std::stringstream ss(paths_string);
		std::vector<std::string> parsed_paths;
		std::string temp_path;

		while (std::getline(ss, temp_path, '\n')) {
			if (!temp_path.empty()) {
				parsed_paths.push_back(temp_path);
			}
		}

		if (op == PathOp::RENAME) {
			if (parsed_paths.size() != 2) {
				message_array.push_back("-1");
				global_success = false;
			} else {
				std::error_code ec;
				fs::path p_src(parsed_paths[0]);
				fs::path p_dst(parsed_paths[1]);

				fs::rename(p_src, p_dst, ec);
				if (!ec) {
					message_array.push_back("0");
				} else {
					message_array.push_back("-1");
					global_success = false;
				}
			}
		} 
		else if (op == PathOp::COPY) {
			if (parsed_paths.size() < 2) {
				message_array.push_back("-1"); //
				global_success = false;
			} else {
				// The destination is always the final string in the array
				fs::path p_dst(parsed_paths.back());
				
				// Using recursive copy, but removing overwrite_existing 
				auto copy_opts = fs::copy_options::recursive;

				// Iterate through all paths except the last one (the destination)
				for (size_t i = 0; i < parsed_paths.size() - 1; ++i) {
					std::error_code ec;
					fs::path p_src(parsed_paths[i]);
					
					// Append the source filename to the destination directory
					fs::path target_path = p_dst / p_src.filename();

					// Handle duplication if the target already exists
					if (fs::exists(target_path)) {
						std::string stem = p_src.stem().string();
						std::string ext = p_src.extension().string();
						
						// First duplication attempt
						target_path = p_dst / (stem + "_copy" + ext);
						
						// Further duplication attempts
						int counter = 1;
						while (fs::exists(target_path)) {
							target_path = p_dst / (stem + "_copy_" + std::to_string(counter) + ext);
							counter++;
						}
					}

					fs::copy(p_src, target_path, copy_opts, ec);
					
					if (!ec) {
						message_array.push_back("0");
					} else {
						message_array.push_back("-1");
						global_success = false;
					}
				}
			}
		}
		// Single-path operations loop
		else {
			for (const auto& target_path : parsed_paths) {
				if (target_path.empty()) continue;

				std::error_code ec;
				fs::path p(target_path);
				bool exists = fs::exists(p, ec);

				switch (op) {
					case PathOp::CREATE: {
						if (exists && fs::is_regular_file(p)) {
							continue;
						}
						if (fs::create_directories(p, ec) || fs::exists(p)) {
							message_array.push_back("0");
						} else {
							message_array.push_back("-1");
							global_success = false;
						}
						break;
					}

					case PathOp::DELETE: {
						if (!exists) {
							message_array.push_back("-1");
							global_success = false;
						} else {
							fs::remove_all(p, ec);
							if (!ec) {
								message_array.push_back("0");
							} else {
								message_array.push_back("-1");
								global_success = false;
							}
						}
						break;
					}

					case PathOp::CHECK: {
						if (exists) {
							message_array.push_back("0");
						} else {
							message_array.push_back("-1");
						}
						global_success = true;
						break;
					}

					case PathOp::LIST: {
						if (exists && fs::is_regular_file(p)) {
							continue;
						}
						if (exists && fs::is_directory(p)) {
							auto options = std::filesystem::directory_options::skip_permission_denied;
							for (const auto& entry : std::filesystem::directory_iterator(p, options)) {
								std::string item_name = entry.path().filename().string();
								if (!item_name.empty() && item_name.front() == '.') continue;

								auto ends_with = [](const std::string& str, const std::string& suffix) {
									return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
								};
								if (ends_with(item_name, "_patched.stl") || ends_with(item_name, "_tmp.stl")) continue;

								if (entry.is_regular_file()) item_name += "|";
								message_array.push_back(item_name);
							}
						}
						break;
					}
					default:
						break;
				}
			}
		}

		response["status"] = global_success ? "success" : "failure";		
		response["message"] = message_array;
		return response;
	}

	void handle_get_file_content(const std::string& path, FileRequestType reqType) {
		json response;
		m_outgoing_buffers.clear();

		if (!fs::exists(path)) {
			response["status"] = "error";
			response["message"] = "Path not found: " + path;
			do_write_string(response.dump() + "\n");
			return;
		}

		if (fs::is_regular_file(path)) {
			unsigned int size = fs::file_size(path);
			response["status"] = "success";
			response["type"] = "file";
			
			// Provide byteSize
			if (reqType == FileRequestType::Stats || reqType == FileRequestType::ContentAndStats) {
				auto ftime = fs::last_write_time(path);
				// Convert C++ filesystem time to a Unix timestamp
				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
								ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
				std::time_t mtime = std::chrono::system_clock::to_time_t(sctp);
				
				response["mtime"] = mtime;
			}

			// Keep existing size payload for legacy content requests[cite: 1]
			if (reqType == FileRequestType::Content || reqType == FileRequestType::ContentAndStats) {
				response["byteSize"] = size; 
			} else if (reqType == FileRequestType::Stats) {
				response["byteSize"] = size;
			}

			m_outgoing_json = response.dump() + "\n";
			m_outgoing_buffers.push_back(asio::buffer(m_outgoing_json));

			// Only execute the expensive disk read if content is actually needed
			if (reqType != FileRequestType::Stats) {
				std::ifstream file(path, std::ios::binary);
				if (file) {
					m_outgoing_file_buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					m_outgoing_buffers.push_back(asio::buffer(m_outgoing_file_buffer));
				}
			}

			// Add the async_write call for the transfer[cite: 1]
			auto self(shared_from_this());
			asio::async_write(m_socket, m_outgoing_buffers,
				[this, self](std::error_code ec, std::size_t /*length*/) {
					if (ec) {
						std::cerr << "File transfer error: " << ec.message() << "\n";
					}
				});
		} else if (fs::is_directory(path)) {
            json response;

            // Handle exceptions
            RenderData renderData;
            try {
                renderData = read_mesh(path);
                if (renderData.data.empty() || renderData.indices.empty()) {
                    throw std::runtime_error("Parsed OpenFOAM arrays are empty.");
                }
            } catch (const std::exception& e) {				
                response["status"] = "error";
                response["message"] = std::string("Failed to parse result data: ") + e.what();
                do_write_string(response.dump() + "\n");
                return;
            }

            // Build the payload
            auto payload = std::make_shared<RenderPayload>();
            payload->vertices = std::move(renderData.data);
            payload->indices = std::move(renderData.indices);
            payload->lineIndices = std::move(renderData.lineIndices);			
			payload->patches = std::move(renderData.patches);

            // Configure the header
			payload->header.magicNumber = 0xFEEDBEEF;
			payload->header.dataByteSize = static_cast<uint32_t>(payload->vertices.size() * sizeof(float));
			payload->header.indexByteSize = static_cast<uint32_t>(payload->indices.size() * sizeof(uint32_t));
			payload->header.lineIndexByteSize = static_cast<uint32_t>(payload->lineIndices.size() * sizeof(uint32_t));			
			payload->header.patchesByteSize = static_cast<uint32_t>(payload->patches.size() * sizeof(RenderPatch));
			payload->header.boundingBoxMin = renderData.boundingBoxMin;
			payload->header.boundingBoxMax = renderData.boundingBoxMax;

			// Populate JSON
			response["status"] = "success";
			response["type"] = "result";
			payload->jsonString = response.dump() + "\n";

			// Queue buffers
			m_outgoing_buffers.clear();
			m_outgoing_buffers.push_back(asio::buffer(payload->jsonString));
			m_outgoing_buffers.push_back(asio::buffer(&payload->header, sizeof(RenderHeader)));
			m_outgoing_buffers.push_back(asio::buffer(payload->vertices));
			m_outgoing_buffers.push_back(asio::buffer(payload->indices));
			m_outgoing_buffers.push_back(asio::buffer(payload->lineIndices));			
			m_outgoing_buffers.push_back(asio::buffer(payload->patches));

            auto self(shared_from_this());
			m_socket.set_option(asio::ip::tcp::no_delay(true));
            asio::async_write(m_socket, m_outgoing_buffers,
                [this, self, payload](std::error_code ec, std::size_t) {
                    if (ec) {
                        std::cerr << "Mesh transfer error: " << ec.message() << "\n";
                    }
                });
			m_socket.set_option(asio::ip::tcp::no_delay(false));
        }
    }

    // A helper for simple string responses
    void do_write_string(std::string response) {
        // asio::post ensures thread safety when interacting with the queue
        asio::post(m_socket.get_executor(), [this, self = shared_from_this(), msg = std::move(response)]() mutable {
            bool write_in_progress = !m_write_queue.empty();
            m_write_queue.push(std::move(msg));

            // If the socket isn't currently busy writing, start the chain
            if (!write_in_progress) {
                do_write_front();
            }
        });
    }

    void do_write_front() {
        auto self(shared_from_this());

        // Write the string currently at the front of the queue
        asio::async_write(m_socket, asio::buffer(m_write_queue.front()),
            [this, self](std::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    m_write_queue.pop(); // Remove the completed message

                    if (!m_write_queue.empty()) {
                        do_write_front(); // Automatically start the next one
                    }
                } else {
                    std::cerr << "Write error: " << ec.message() << "\n";
                }
            });
    }

	json handle_get_times_fields(const std::string& path_str) {
		json result;
		std::filesystem::path target_path(path_str);
		std::error_code ec;

		// Verify the path exists and is a directory
		if (!std::filesystem::exists(target_path, ec) || !std::filesystem::is_directory(target_path, ec)) {
			result["status"] = "error";
			result["message"] = "Directory does not exist or cannot be accessed.";
			return result;
		}

		auto options = std::filesystem::directory_options::skip_permission_denied;
		std::map<double, std::string> sorted_folders;

		// Iterate through root directory
		for (const auto& entry : std::filesystem::directory_iterator(target_path, options, ec)) {
			if (entry.is_directory(ec)) {
				std::string dir_name = entry.path().filename().string();
				
				try {
					size_t pos = 0;
					double time_val = std::stod(dir_name, &pos);
					
					// Ensure the entire string was parsed
					if (pos == dir_name.length()) {
						sorted_folders[time_val] = dir_name;
					}
				} catch (const std::exception&) {
					// Ignore non-numeric directories like 'system' or 'constant'
				}
			}
		}

		// Populate JSON array with time folders
		json time_folders = json::array();
		for (const auto& [time_val, dir_name] : sorted_folders) {
			time_folders.push_back(dir_name);
		}

		json latest_fields = json::array();
		
		// Find field files in the latest time directory
		if (!sorted_folders.empty()) {
			std::string latest_time_dir = sorted_folders.rbegin()->second;
			std::filesystem::path latest_time_path = target_path / latest_time_dir;
			
			if (std::filesystem::exists(latest_time_path, ec) && std::filesystem::is_directory(latest_time_path, ec)) {
				for (const auto& entry : std::filesystem::directory_iterator(latest_time_path, options, ec)) {
					if (entry.is_regular_file(ec)) {
						latest_fields.push_back(entry.path().filename().string());
					}
				}
			}
		}

		// Return structured JSON data instead of a delimited string
		result["status"] = "success";
		result["time_folders"] = time_folders;
		result["latest_fields"] = latest_fields;
		return result;
	}

    json handle_launch_short_utility(std::string cmd) {

        json response;
        std::string outputText;

        // Construct the command: change directory, run bash, and capture stderr to stdout
        std::string full_cmd = "bash -c '" + cmd + "' 2>&1";

        // Open a pipe to read the output of the executed command
        FILE* pipe = popen(full_cmd.c_str(), "r");
        if (!pipe) {
            response["status"] = "error";
            response["message"] = "popen() failed to start the process!";
            response["exitCode"] = -1;
            return response;
        }

        // Read the output chunk by chunk and append to the single string
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            outputText += buffer;
        }

        // Close the pipe and get the exit status of the command
        int returnCode = pclose(pipe);

        // WEXITSTATUS macro is typically needed on Linux/WSL to get the actual exit code from pclose
        int exitStatus = WEXITSTATUS(returnCode);

        response["status"] = (exitStatus == 0) ? "success" : "error";
        response["exitCode"] = exitStatus;
        response["message"] = outputText;
        return response;
    }

    void handle_launch_long_utility(std::string cmd) {
        // Capture the shared pointer
        auto self(shared_from_this());

        // Spawn a background thread
        std::thread([this, self, cmd]() {
            std::string full_cmd = "bash -c '" + cmd + "' 2>&1";
            FILE* pipe = popen(full_cmd.c_str(), "r");

            if (!pipe) {
                return;
            }

            char buffer[1024];

            // Read the output
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string chunk(buffer);

                // Format a JSON update indicating the process is still running
                json update;
                update["status"] = "running";
                update["output"] = chunk;

                // Push the write operation back to the ASIO event loop
                asio::post(m_socket.get_executor(), [this, self, update_str = update.dump() + "\n"]() {
                    do_write_string(update_str);
                });
            }

            // Command finished, get exit code
            int returnCode = pclose(pipe);
            int exitStatus = WEXITSTATUS(returnCode);

            // Format final success/error JSON
            json final_msg;
            final_msg["status"] = (exitStatus == 0) ? "success" : "error";
            final_msg["exitCode"] = exitStatus;

            // Safely send the final message
            asio::post(m_socket.get_executor(), [this, self, final_str = final_msg.dump() + "\n"]() {
                do_write_string(final_str);
            });

        }).detach();
    }

    json handle_write_data(std::string path, size_t byteSize) {
        // Check for the flag and strip it from the path
        bool make_executable = false;
        if (!path.empty() && path.back() == '|') {
            make_executable = true;
            path.pop_back();
        }

        // Create directories as needed
        fs::path targetPath(path);
        fs::create_directories(targetPath.parent_path());

        // Create the new file
        std::ofstream outfile(path, std::ios::binary);
        json response;

        if (!outfile) {
            response["status"] = "error";
            response["message"] = "Failed to open file on the server.";
        } else {
            size_t bytes_received = m_data.size();
            if (bytes_received > 0) {
                outfile.write(m_data.data(), bytes_received);
                m_data.clear();
            }

            // Stream the remaining binary chunks
            size_t bytes_to_read = byteSize - bytes_received;
            char buffer[65536];
            asio::error_code ec;

            while (bytes_to_read > 0) {
                size_t chunk_size = std::min(bytes_to_read, sizeof(buffer));

                // Use asio::read to block until 'chunk_size' bytes are received
                size_t read_bytes = asio::read(m_socket, asio::buffer(buffer, chunk_size), ec);

                if (ec && ec != asio::error::eof) {
                    response["status"] = "error";
                    response["message"] = "Socket error during binary transfer: " + ec.message();
                    break;
                }
                outfile.write(buffer, read_bytes);
                bytes_to_read -= read_bytes;
            }

            if (!ec || ec == asio::error::eof) {
                outfile.close();
                if (make_executable) {
                    std::error_code perm_ec;
                    fs::permissions(
                        targetPath,
                        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                        fs::perm_options::add, perm_ec
                    );
                }
                response["status"] = "success";
            }
        }
        return response;
    }

    json handle_check_openfoam() {

        json result;
        result["status"] = "success";
        result["message"] = json::array();

        std::vector<fs::path> base_paths = {"/usr/lib/openfoam", "/opt"};
        for (const auto& base : base_paths) {
            if (fs::exists(base) && fs::is_directory(base)) {
                for (const auto& entry : fs::directory_iterator(base)) {
                    std::string dirname = entry.path().filename().string();
                    if (entry.is_directory() && dirname.rfind("openfoam", 0) == 0) {
                        result["message"].push_back(entry.path().string());
                    }
                }
            }
        }
        return result;
    }

    json handle_find_tutorials(const std::string& base_path) {
        json result;
        result["status"] = "success";
        result["message"] = json::array();

        fs::path target_path = fs::path(base_path) / "tutorials";

        if (!fs::exists(target_path) || !fs::is_directory(target_path)) {
            result["status"] = "error";
            result["message"].push_back("Tutorials path does not exist.");
            return result;
        }

        // Iterate recursively to find "system" directories
        for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
            if (entry.is_directory() && entry.path().filename() == "system") {
                result["message"].push_back(entry.path().parent_path().string());
            }
        }
        return result;
    }

	// ZLIB extraction
	bool extractGzGeometry(const std::string& gzFilePath, const std::string& outFilePath) {
		gzFile inFile = gzopen(gzFilePath.c_str(), "rb");
		if (!inFile) return false;

		std::ofstream outFile(outFilePath, std::ios::binary);
		if (!outFile.is_open()) {
			gzclose(inFile);
			return false;
		}

		const int bufferSize = 128 * 1024;
		std::vector<char> buffer(bufferSize);
		int bytesRead = 0;

		while ((bytesRead = gzread(inFile, buffer.data(), bufferSize)) > 0) {
			outFile.write(buffer.data(), bytesRead);
		}

		gzclose(inFile);
		outFile.close();
		return (bytesRead >= 0);
	}

	// Evaluate wildcards (*, ?)
	bool matchGlob(const std::string& pattern, const std::string& text) {
		std::string regexPattern = "^";
		for (char c : pattern) {
			switch (c) {
				case '*': regexPattern += ".*"; break;
				case '?': regexPattern += "."; break;
				// Escape special characters
				case '.': case '+': case '(': case ')': case '[': case ']':
				case '{': case '}': case '^': case '$': case '|': case '\\':
					regexPattern += '\\';
					regexPattern += c;
					break;
				default:
					regexPattern += c;
					break;
			}
		}
		regexPattern += "$";
		try {
			return std::regex_match(text, std::regex(regexPattern));
		} catch (...) {
			return false;
		}
	}

	void processAllrunScript(const fs::path& scriptPath, const fs::path& projectPath, 
		const fs::path& originalTutorialPath, json& result) {
			
		if (!fs::exists(scriptPath)) return;
		std::ifstream file(scriptPath);
		if (!file.is_open()) return;
		
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string content = buffer.str();

		// Remove line continuations
		std::regex continuationRegex(R"(\\\n\s*)");
		content = std::regex_replace(content, continuationRegex, "");

		// Find the base "tutorials" folder
		fs::path tutorialsBase;
		fs::path current = originalTutorialPath;
		while (current.has_parent_path()) {
			if (current.filename() == "tutorials") {
				tutorialsBase = current;
				break;
			}
			current = current.parent_path();
		}

		if (tutorialsBase.empty()) {
			result["status"] = "warning";
			result["status_message"].push_back("Could not deduce base 'tutorials' directory from: " + originalTutorialPath.string());
			return;
		}

		// Regex to find 'cp' commands targeting resources/geometry
		std::regex cpRegex(R"(cp\s+(?:-[A-Za-z]+\s+)*"?\$FOAM_TUTORIALS"?/resources/geometry/([^\s]+)\s+([^\s]+))");
		std::smatch match;

		std::string::const_iterator searchStart(content.cbegin());
		while (std::regex_search(searchStart, content.cend(), match, cpRegex)) {
			std::string geomPattern = match[1].str();
			std::string destDirStr = match[2].str();

			fs::path sourceDir = tutorialsBase / "resources" / "geometry";
			fs::path destDir = projectPath / destDirStr;

			if (!fs::exists(destDir)) {
				fs::create_directories(destDir);
			}

			// Ensure the resources/geometry directory actually exists before iterating
			if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
				result["status"] = "warning";
				result["status_message"].push_back("Geometry resources directory not found: " + sourceDir.string());
				searchStart = match.suffix().first;
				continue;
			}

			bool fileMatched = false;

			// Iterate through the resources/geometry directory to find matching files
			for (const auto& entry : fs::directory_iterator(sourceDir)) {
				std::string filename = entry.path().filename().string();

				// Check if the file matches the script's pattern (exact or wildcard)
				if (matchGlob(geomPattern, filename)) {
					fileMatched = true;
					fs::path sourcePath = entry.path();

					// Handle Directory, GZ file, or Standard File
					if (fs::is_directory(sourcePath)) {
						fs::copy(sourcePath, destDir / filename, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
						result["status_message"].push_back("Copied geometry dir: " + filename);
					} else if (sourcePath.extension() == ".gz") {
						fs::path outPath = destDir / sourcePath.stem();
						if (extractGzGeometry(sourcePath.string(), outPath.string())) {
							result["status_message"].push_back("Decompressed: " + outPath.filename().string());
						} else {
							result["status"] = "warning";
							result["status_message"].push_back("Failed to decompress: " + filename);
						}
					} else {
						fs::copy_file(sourcePath, destDir / filename, fs::copy_options::overwrite_existing);
						result["status_message"].push_back("Copied geometry file: " + filename);
					}
				}
			}

			if (!fileMatched) {
				result["status"] = "warning";
				result["status_message"].push_back("No geometry files matched pattern: " + geomPattern);
			}

			searchStart = match.suffix().first;
		}
	}

	// Copy tutorials
	json handle_copy_tutorials(const std::string& path1, const std::string& path2) {
		json result;
		result["status"] = "success";
		result["status_message"] = json::array();		
		result["message"] = json::array();

		fs::path tutorial_path = fs::path(path1);
		fs::path project_path = fs::path(path2);

		if (!fs::exists(tutorial_path) || !fs::is_directory(tutorial_path)) {
			result["status"] = "error";
			result["status_message"].push_back("Tutorials path does not exist or is not a directory.");
			return result;
		}

		try {
			if (!fs::exists(project_path)) {
				fs::create_directories(project_path);
			} else if (fs::is_directory(project_path)) {
				for (const auto& entry : fs::directory_iterator(project_path)) {
					fs::remove_all(entry.path());
				}
			} else {
				result["status"] = "error";
				result["status_message"].push_back("Project path exists but is not a directory.");
				return result;
			}
		} catch (const fs::filesystem_error& e) {
			result["status"] = "error";
			result["status_message"].push_back("Failed to prepare project directory. Error: " + std::string(e.what()));
			return result;
		}

		// Essential files to copy
		std::vector<std::string> items = { "0", "0.orig", "constant", "system", "Allrun", "Allrun.pre", "Allclean" };

		const auto copyOptions = fs::copy_options::recursive | fs::copy_options::overwrite_existing;
		for (const auto& item_name : items) {
			fs::path source_item = tutorial_path / item_name;
			fs::path dest_item   = project_path / item_name;

			if (fs::exists(source_item)) {
				try {
					fs::copy(source_item, dest_item, copyOptions);
					std::string formatted_name = item_name;
					if (fs::is_regular_file(source_item)) {
						formatted_name += "|";
					}
					result["message"].push_back(formatted_name);
				} catch (const fs::filesystem_error& e) {
					result["status"] = "error";
					result["status_message"].push_back("Failed to copy " + item_name + ". Error: " + std::string(e.what()));
					return result;
				}
			}
		}

		// Parse both scripts to auto-resolve geometry dependencies
		processAllrunScript(project_path / "Allrun", project_path, tutorial_path, result);
		processAllrunScript(project_path / "Allrun.pre", project_path, tutorial_path, result);

		if (!fs::exists(project_path / "system")) {
			result["status"] = "warning";
			result["status_message"].push_back("Warning: No 'system' directory was found in the tutorial folder.");
		}
		if (fs::exists(project_path / "constant") && 
		    std::find(result["message"].begin(), result["message"].end(), "constant") == result["message"].end()) {			
			result["message"].insert(result["message"].begin(), "constant");
		}
		return result;
	}

	tcp::socket m_socket;
	std::string m_data;
};

class WslServer {
public:
    WslServer(asio::io_context& io_context, short port)
        : m_acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        m_acceptor.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Create and start the session, safely managed by shared_ptr
                    std::make_shared<WslSession>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor m_acceptor;
};

int main() {
    try {
        asio::io_context io_context;
        WslServer server(io_context, 53626);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}