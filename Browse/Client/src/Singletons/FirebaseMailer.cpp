//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "FirebaseMailer.h"
#include "src/Utils/glmWrapper.h"
#include "src/Utils/Logger.h"
#include "src/Utils/Helper.h"
#include "externals/curl/include/curl/curl.h"
#include <sstream>
#include <chrono>

using json = nlohmann::json;

// ########################
// ### TEMPLATE HELPERS ###
// ########################

// Fallback for values when not found in database etc.
template<typename Type> Type fallback();
template<> int fallback<int>()					{ return 0; };
template<> std::string fallback<std::string>()	{ return ""; };
template<> json fallback<json>()				{ return json(); };

// ####################
// ### HTTP HELPERS ###
// ####################

// Write callback for CURL
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

// Checks for something in the first line of HTTP header
bool HttpHeaderFirstLineContains(const std::string& rHeader, const std::string& rCompareTo)
{
	std::string firstLine = rHeader.substr(0, rHeader.find("\n"));
	return firstLine.find(rCompareTo) != std::string::npos;
}

// Checks header for ok
bool HttpHeaderOK(const std::string& rHeader)
{
	return HttpHeaderFirstLineContains(rHeader, "200 OK")
		|| HttpHeaderFirstLineContains(rHeader, "100 Continue"); // Google sometimes returns this and in the third line 200. For now, treating as synonyms
}

// Checks header for failed precondition
bool HttpHeaderPreconditionFailed(const std::string& rHeader)
{
	return HttpHeaderFirstLineContains(rHeader, "412 Precondition Failed");
}

// Extract ETag from header, returns empty string if not found
std::string HttpHeaderExtractETag(const std::string& rHeader)
{
	std::istringstream lineStream(rHeader);
	std::string line;
	std::getline(lineStream, line); // first line is not of interest
	while(std::getline(lineStream, line)) // go over lines
	{
		std::istringstream tokenStream(line);
		std::string token;
		std::vector<std::string> tokens;
		while (std::getline(tokenStream, token, ':')) // go over tokens
		{
			tokens.push_back(token); // collect tokens
		}

		// Should be two tokens, and first should be "ETag"
		if (tokens.size() == 2 && tokens.at(0) == "ETag")
		{
			// Extract second token as ETag
			return tokens.at(1).substr(1, tokens.at(1).length() - 1); // return without preceeding space
		}
	}

	return ""; // return empty string in case of non exisiting ETag
}

// ##########################
// ### FIREBASE INTERFACE ###
// ##########################

bool FirebaseMailer::FirebaseInterface::Login(std::string email, std::string password, bool useDriftMap, std::promise<std::string>* pPromise)
{
	// Store email and password
	_email = email;
	_password = password;

	// Perform actual login
	bool success = Login();

	// If success, retrieve start index
	if (success)
	{
		std::promise<int> promise; auto future = promise.get_future(); // future provides index
		Transform(FirebaseIntegerKey::GENERAL_APPLICATION_START_COUNT, 1, &promise); // adds one to the count
		int index = future.get() - 1;
		nlohmann::json record = {
			{ "date", GetDate() }, // add date
			{ "timestamp", GetTimestamp() }, // add timestamp
			{ "version", CLIENT_VERSION }, // add version
			{ "useDriftMap", useDriftMap } // add information whether drift map is used
		};
		Put(FirebaseJSONKey::GENERAL_APPLICATION_START, record, std::to_string(index)); // send JSON to database
		*_pStartIndex = index;
	}

	// Fullfill the promise
	pPromise->set_value(_pIdToken->Get());

	// Return the success
	return success;
}

template<typename T>
void FirebaseMailer::FirebaseInterface::Put(T key, typename FirebaseValue<T>::type value, std::string subpath)
{
	// Only continue if logged in
	bool success = false;
	if (_pIdToken->IsSet())
	{
		// Setup CURL
		CURL *curl;
		curl = curl_easy_init();

		// Continue if CURL was initialized
		if (curl)
		{
			// Local variables
			std::string answerHeaderBuffer;
			std::string answerBodyBuffer;

			// Request URL
			std::string requestURL =
				_URL + "/"
				+ BuildFirebaseKey(key, _uid) + "/" + subpath
				+ ".json"
				+ "?auth=" + _pIdToken->Get();

			// Setup CURL
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // tell about PUTting
			curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L); // CURL told me to use it
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback); // set callback for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // set callback for answer body
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &answerHeaderBuffer); // set buffer for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerBodyBuffer); // set buffer for answer body

			// Post field
			const std::string postField = json(value).dump();

			// Fill request
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField.c_str()); // value to put
			curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of request

			 // Perform the request
			CURLcode res = curl_easy_perform(curl);
			if (res == CURLE_OK) // everything ok with CURL
			{
				// Check header
				if (HttpHeaderOK(answerHeaderBuffer))
				{
					success = true; // fine!
				}
				else // there is something else in header
				{
					// Try to relogin
					if (Relogin()) // returns whether successful
					{
						// Setup CURL request (reuse stuff from first)
						answerHeaderBuffer.clear();
						answerBodyBuffer.clear();
						requestURL =
							_URL + "/"
							+ BuildFirebaseKey(key, _uid) + "/" + subpath
							+ ".json"
							+ "?auth=" + _pIdToken->Get();
						curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of new request

						// Try again to PUT data
						res = curl_easy_perform(curl);
						if (res == CURLE_OK) // everything ok with CURL
						{
							if (HttpHeaderOK(answerHeaderBuffer))
							{
								success = true; // fine!
							}
							// else: ok, it failed.
						}
					}
				}
			}

			// Cleanup
			curl_easy_cleanup(curl); curl = nullptr;
		}
	}

	// Tell user about no success
	if (!success)
	{
		LogError("FirebaseInterface: ", "Data transfer to Firebase failed.");
	}
}

template<typename T>
void FirebaseMailer::FirebaseInterface::Get(T key, std::promise<typename FirebaseValue<T>::type>* pPromise)
{
	auto result = Get(BuildFirebaseKey(key, _uid));
	if(!result.value.empty()) // result might be empty and json does not like to convert empty stuff
	{
		pPromise->set_value(result.value.get<typename FirebaseValue<T>::type>());
	}
	else
	{
		pPromise->set_value(fallback<typename FirebaseValue<T>::type>()); // for empty result, just use fallback
	}
}

void FirebaseMailer::FirebaseInterface::Transform(FirebaseIntegerKey key, int delta, std::promise<int>* pPromise)
{
	// Just add the delta to the existing value
	auto result = Apply(key, [delta](int DBvalue) { return DBvalue + delta; });
	if (pPromise != nullptr) { pPromise->set_value(result); }
}

void FirebaseMailer::FirebaseInterface::Maximum(FirebaseIntegerKey key, int value, std::promise<int>* pPromise)
{
	// Use maximum of database value and this
	auto result = Apply(key, [value](int DBvalue) { return glm::max(DBvalue, value); });
	if (pPromise != nullptr) { pPromise->set_value(result); }
}

bool FirebaseMailer::FirebaseInterface::Login()
{
	bool success = false;

	// Invalidate tokens
	_pIdToken->Reset();
	_refreshToken = "";

	// Setup CURL
	CURL *curl;
	curl = curl_easy_init();

	// Continue if CURL was initialized
	if (curl)
	{
		// Local variables
		CURLcode res;
		std::string postBuffer;
		std::string answerHeaderBuffer;
		std::string answerBodyBuffer;
		struct curl_slist* headers = nullptr; // init to NULL is important 
		headers = curl_slist_append(headers, "Content-Type: application/json"); // type is JSON

		// Set options
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // apply header
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback); // set callback for answer header
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // set callback for answer body
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &answerHeaderBuffer); // set buffer for answer header
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerBodyBuffer); // set buffer for answer body

		// Post field
		const json jsonPost =
		{
			{ "email", _email }, // email of user
			{ "password", _password }, // password to access database
			{ "returnSecureToken", true } // of course, thats what this is about
		};
		postBuffer = jsonPost.dump(); // store in a string, otherwise it breaks (CURL probably wants a reference)

									  // Fill request
		curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/identitytoolkit/v3/relyingparty/verifyPassword?key=" + _API_KEY); // URL to access
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBuffer); // fill post body

		// Execute request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) // something went wrong
		{
			LogError("FirebaseInterface: ", "User login to Firebase failed: ", curl_easy_strerror(res));
			return false;
		}

		// Cleanup
		curl_slist_free_all(headers); headers = nullptr;
		curl_easy_cleanup(curl); curl = nullptr;

		// Parse answer to JSON object and extract id token
		const auto jsonAnswer = json::parse(answerBodyBuffer);
		auto result = jsonAnswer.find("idToken");
		if (result != jsonAnswer.end())
		{
			_pIdToken->Set(result.value().get<std::string>());
			LogInfo("FirebaseInterface: ", "User successfully logged into Firebase.");
			success = true;
		}
		else
		{

			LogError("FirebaseInterface: ", "User login to Firebase failed.");
		}

		// Search for refresh token (optional)
		result = jsonAnswer.find("refreshToken");
		if (result != jsonAnswer.end())
		{
			_refreshToken = result.value().get<std::string>();
		}

		// Search for uid
		result = jsonAnswer.find("localId");
		if (result != jsonAnswer.end())
		{
			_uid = result.value().get<std::string>(); // one could "break mailer" if not provided as nothing makes sense
		}
	}
	return success;
}

bool FirebaseMailer::FirebaseInterface::Relogin()
{
	bool success = false;

	// Store refresh token locally
	std::string refreshToken = _refreshToken;

	// Invalidate tokens
	_pIdToken->Reset();
	_refreshToken = "";

	// Check for empty refresh token
	if (refreshToken.empty())
	{
		// No relogin through refresh token possible, try complete relogin
		success = Login();
	}
	else // relogin possilbe
	{
		// Setup CURL
		CURL *curl;
		curl = curl_easy_init();

		// Continue if CURL was initialized
		if (curl)
		{
			// Local variables
			CURLcode res;
			std::string answerBodyBuffer;
			struct curl_slist* headers = nullptr; // init to NULL is important 
			headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded"); // type is simple form encoding

			// Set options
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // apply header
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // set callback for answer body
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerBodyBuffer); // set buffer for answer body

			// Post field
			const std::string postBuffer = "grant_type=refresh_token&refresh_token=" + refreshToken;

			// Fill request
			curl_easy_setopt(curl, CURLOPT_URL, "https://securetoken.googleapis.com/v1/token?key=" + _API_KEY); // URL to access
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBuffer); // fill post body

			// Execute request
			res = curl_easy_perform(curl);
			if (res != CURLE_OK) // something went wrong
			{
				LogError("FirebaseInterface: ", "User reauthentifiation to Firebase failed: ", curl_easy_strerror(res));
				return false;
			}

			// Cleanup
			curl_slist_free_all(headers); headers = nullptr;
			curl_easy_cleanup(curl); curl = nullptr;

			// Parse answer to JSON object and extract id token
			const auto jsonAnswer = json::parse(answerBodyBuffer);
			auto result = jsonAnswer.find("id_token"); // different from email and password login
			if (result != jsonAnswer.end())
			{
				_pIdToken->Set(result.value().get<std::string>());
				LogInfo("FirebaseInterface: ", "User reauthentifiation to Firebase successful.");
				success = true;
			}
			else
			{
				LogError("FirebaseInterface: ", "User reauthentifiation to Firebase failed.");
			}

			// Search for refresh token (optional)
			result = jsonAnswer.find("refresh_token");
			if (result != jsonAnswer.end())
			{
				_refreshToken = result.value().get<std::string>();
			}
		}
	}

	return success;
}

template<typename T>
bool FirebaseMailer::FirebaseInterface::Put(T key, std::string ETag, typename FirebaseValue<T>::type value, std::string& rNewETag, typename FirebaseValue<T>::type& rNewValue, std::string subpath)
{
	bool success = false;
	rNewETag = "";
	rNewValue = fallback<typename FirebaseValue<T>::type>();

	// Only continue if logged in
	if (_pIdToken->IsSet())
	{
		// Setup CURL
		CURL *curl;
		curl = curl_easy_init();

		// Continue if CURL was initialized
		if (curl)
		{
			// Local variables
			std::string answerHeaderBuffer;
			std::string answerBodyBuffer;
			struct curl_slist* headers = nullptr; // init to NULL is important
			const std::string putHeaderBuffer = "if-match:" + ETag;
			headers = curl_slist_append(headers, putHeaderBuffer.c_str()); // 'if-match' criteria to use the ETag

			// Request URL
			std::string requestURL =
				_URL + "/"
				+ BuildFirebaseKey(key, _uid) + "/" + subpath
				+ ".json"
				+ "?auth=" + _pIdToken->Get();

			// Setup CURL
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // apply header
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT"); // tell about PUTting
			curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L); // CURL told me to use it
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback); // set callback for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // set callback for answer body
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &answerHeaderBuffer); // set buffer for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerBodyBuffer); // set buffer for answer body

			// Post field
			const std::string postField = json(value).dump();

			// Fill request
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postField.c_str()); // value to put
			curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of request

			// Perform the request
			CURLcode res = curl_easy_perform(curl);
			if (res == CURLE_OK) // everything ok with CURL
			{
				// Check header
				if (HttpHeaderOK(answerHeaderBuffer))
				{
					success = true; // fine!
				}
				else if (HttpHeaderPreconditionFailed(answerHeaderBuffer))
				{
					// Fill newETag and newValue
					rNewETag = HttpHeaderExtractETag(answerHeaderBuffer);
					rNewValue = json::parse(answerBodyBuffer).get<typename FirebaseValue<T>::type>();
				}
				else // there is something else in header
				{
					// Try to relogin
					if (Relogin()) // returns whether successful
					{
						// Setup CURL request (reuse stuff from first)
						answerHeaderBuffer.clear();
						answerBodyBuffer.clear();
						requestURL =
							_URL + "/"
							+ BuildFirebaseKey(key, _uid) + "/" + subpath
							+ ".json"
							+ "?auth=" + _pIdToken->Get();
						curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of new request

						// Try again to PUT data
						res = curl_easy_perform(curl);
						if (res == CURLE_OK) // everything ok with CURL
						{
							if (HttpHeaderOK(answerHeaderBuffer))
							{
								success = true; // fine!
							}
							else if (HttpHeaderPreconditionFailed(answerHeaderBuffer))
							{
								// Fill newETag and newValue
								rNewETag = HttpHeaderExtractETag(answerHeaderBuffer);
								rNewValue = json::parse(answerBodyBuffer).get<typename FirebaseValue<T>::type>();
							}
							// else: ok, it failed.
						}
					}
				}
			}

			// Cleanup
			curl_slist_free_all(headers); headers = nullptr;
			curl_easy_cleanup(curl); curl = nullptr;
		}
	}

	return success;
}

FirebaseMailer::FirebaseInterface::DBEntry FirebaseMailer::FirebaseInterface::Get(std::string key)
{
	// Return value
	DBEntry result;

	// Only continue if logged in
	if (_pIdToken->IsSet())
	{
		// Setup CURL
		CURL *curl;
		curl = curl_easy_init();

		// Continue if CURL was initialized
		if (curl)
		{
			// Local variables
			std::string answerHeaderBuffer;
			std::string answerBodyBuffer;
			struct curl_slist* headers = nullptr; // init to NULL is important 
			headers = curl_slist_append(headers, "X-Firebase-ETag: true"); // tell it to deliver ETag to identify the state

			// Request URL
			std::string requestURL =
				_URL + "/"
				+ key
				+ ".json"
				+ "?auth=" + _pIdToken->Get();

			// Setup CURL
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // apply header
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow potential redirection
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback); // set callback for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // set callback for answer body
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &answerHeaderBuffer); // set buffer for answer header
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerBodyBuffer); // set buffer for answer body
			curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of request

			// Perform the request
			CURLcode res = curl_easy_perform(curl);
			if (res == CURLE_OK) // everything ok with CURL
			{
				// Check header for OK
				if (HttpHeaderOK(answerHeaderBuffer))
				{
					// Parse
					result = DBEntry(HttpHeaderExtractETag(answerHeaderBuffer), json::parse(answerBodyBuffer));
				}
				else // not ok, guess timeout of id token?
				{
					// Try to relogin
					if (Relogin()) // returns whether successful
					{
						// Setup CURL request (reuse stuff from first)
						answerHeaderBuffer.clear();
						answerBodyBuffer.clear();
						requestURL =
							_URL + "/"
							+ key
							+ ".json"
							+ "?auth=" + _pIdToken->Get(); // update request URL as id token should be new
						curl_easy_setopt(curl, CURLOPT_URL, requestURL.c_str()); // set address of new request

						// Try again to GET data
						res = curl_easy_perform(curl);
						if (res == CURLE_OK && HttpHeaderOK(answerHeaderBuffer)) // everything ok with CURL and header
						{
							// Parse
							result = DBEntry(HttpHeaderExtractETag(answerHeaderBuffer), json::parse(answerBodyBuffer));
						}
					}
				}
			}

			// Cleanup of CURL
			curl_easy_cleanup(curl); curl = nullptr;
		}
	}

	// Return result
	return result;
}

int FirebaseMailer::FirebaseInterface::Apply(FirebaseIntegerKey key, std::function<int(int)> function)
{
	bool success = false;

	// Call own get method
	const auto entry = this->Get(BuildFirebaseKey(key, _uid));

	// Check whether data was found, create new with fallback value, if necessary
	int value = (!entry.value.empty() && entry.value.is_number_integer()) ? entry.value.get<int>() : fallback<int>();

	// Apply function on the value
	value = function(value);

	// Put value in database
	std::string ETag = entry.ETag;
	std::string newETag = "";
	int newValue = 0;
	const int maxTrialCount = 10;
	int trialCount = 0;

	// Try as long as no success but still new ETags
	do
	{
		// Try to put on database
		newETag = "";
		newValue = 0;
		success = Put(key, ETag, value, newETag, newValue);

		// If no new ETag provided, just quit this (either success or database just does not like us)
		if (newETag.empty()) { break; }

		// Store new values
		ETag = newETag;
		value = newValue;

		// Transform retrieved value
		value = function(value);

		// Increase trial count
		++trialCount;

	} while (!success && trialCount <= maxTrialCount);

	// Tell user about no success
	if (!success)
	{
		LogError("FirebaseInterface: ", "Data transfer to Firebase failed.");
	}

	return value;
}

// #######################
// ### FIREBASE MAILER ###
// #######################

FirebaseMailer::FirebaseMailer()
{
	// Create thread where FirebaseInterface lives in
	auto* pMutex = &_commandMutex;
	auto* pConditionVariable = &_conditionVariable;
	auto* pCommandQueue = &_commandQueue;
	auto* pIdToken = &_idToken;
	auto* pStartIndex = &_startIndex;
	auto const * pShouldStop = &_shouldStop; // read-only
	_upThread = std::unique_ptr<std::thread>(new std::thread([pMutex, pConditionVariable, pCommandQueue, pIdToken, pStartIndex, pShouldStop]() // pass copies of pointers to members
	{
		// Create interface to firebase
		FirebaseInterface interface(pIdToken, pStartIndex); // object of inner class

		// Local command queue where command are moved from mailer thread to this thread
		std::deque<std::shared_ptr<Command> > localCommandQueue;

		// While loop to work on commands
		while (!*pShouldStop) // exits when should stop, so thread can be joined
		{
			// Wait for data
			{
				std::unique_lock<std::mutex> lock(*pMutex); // acquire lock
				pConditionVariable->wait(
					lock, // lock that is handled by condition variable
					[pCommandQueue, pShouldStop]
					{
						return !pCommandQueue->empty() || *pShouldStop; // condition is either there are some commands or it is called to stop and becomes joinable
					}); // hand over locking to the conidition variable which waits for notification by main thread
				localCommandQueue = std::move(*pCommandQueue); // move content of command queue to local one
				pCommandQueue->clear(); // clear original queue
				lock.unlock(); // do not trust the scope stuff. But should be not necessary
			}

			// Work on commands
			for (const auto& rCommand : localCommandQueue)
			{
				(*rCommand.get())(interface);
			}
		}

		// Collect last commands before shutdown
		std::unique_lock<std::mutex> lock(*pMutex);
		localCommandQueue = std::move(*pCommandQueue); // move content of command queue to local one
		pCommandQueue->clear(); // clear original queue

		// Work on these last commands
		for (const auto& rCommand : localCommandQueue)
		{
			(*rCommand.get())(interface);
		}
	}));
}

bool FirebaseMailer::PushBack_Login(std::string email, std::string password, bool useDriftMap, std::promise<std::string>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		return rInterface.Login(email, password, useDriftMap, pPromise);
	})));
}

bool FirebaseMailer::PushBack_Transform(FirebaseIntegerKey key, int delta, std::promise<int>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Transform(key, delta, pPromise);
	})));
}

bool FirebaseMailer::PushBack_Maximum(FirebaseIntegerKey key, int value, std::promise<int>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Maximum(key, value, pPromise);
	})));
}

bool FirebaseMailer::PushBack_Put(FirebaseIntegerKey key, int value, std::string subpath)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Put(key, value, subpath);
	})));
}

bool FirebaseMailer::PushBack_Put(FirebaseStringKey key, std::string value, std::string subpath)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Put(key, value, subpath);
	})));
}

bool FirebaseMailer::PushBack_Put(FirebaseJSONKey key, json value, std::string subpath)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Put(key, value, subpath);
	})));
}

bool FirebaseMailer::PushBack_Get(FirebaseIntegerKey key, std::promise<int>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Get(key, pPromise);
	})));
}

bool FirebaseMailer::PushBack_Get(FirebaseStringKey key, std::promise<std::string>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Get(key, pPromise);
	})));
}

bool FirebaseMailer::PushBack_Get(FirebaseJSONKey key, std::promise<json>* pPromise)
{
	// Add command to queue, take parameters as copy
	return PushBackCommand(std::shared_ptr<Command>(new Command([=](FirebaseInterface& rInterface)
	{
		rInterface.Get(key, pPromise);
	})));
}

bool FirebaseMailer::PushBackCommand(std::shared_ptr<Command> spCommand)
{
	if (setup::FIREBASE_MAILING && !_paused) // only push back the command if logging activated and not paused
	{
		std::lock_guard<std::mutex> lock(_commandMutex);
		_commandQueue.push_back(spCommand); // push back command to queue
		_conditionVariable.notify_all(); // notify thread about new data
		return true;
	}
	else
	{
		return false; // command was not added to the queue
	}
}

std::string FirebaseMailer::GetIdToken() const
{
	// Only read in FirebaseMailer, do not set!
	return _idToken.Get();
}

int FirebaseMailer::GetStartIndex() const
{
	return _startIndex;
}