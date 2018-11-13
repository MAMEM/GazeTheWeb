//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Singleton which receives and sends data to Firebase. The mailer manages
// the command queue and the interface the connection to the Firebase.
// Mailer access is threadsafe.

#ifndef FIREBASEMAILER_H_
#define FIREBASEMAILER_H_

#include "src/Setup.h"
#include "src/Award.h"
#include "submodules/json/src/json.hpp"
#include <string>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

// Available database keys
enum class FirebaseIntegerKey	{ 
	PAGE_ACTIVITY_UNKNOWN_COUNT,
	PAGE_ACTIVITY_NEWS_COUNT,
	PAGE_ACTIVITY_SHOPPING_COUNT,
	PAGE_ACTIVITY_EMAIL_COUNT,
	PAGE_ACTIVITY_JOB_COUNT,
	PAGE_ACTIVITY_ENTERTAINMENT_COUNT,
	PAGE_ACTIVITY_FORUM_COUNT,
	PAGE_ACTIVITY_HEALTH_COUNT,
	PAGE_ACTIVITY_ELEARNING_COUNT,
	PAGE_ACTIVITY_WIKIPEDIA_COUNT,
	PAGE_ACTIVITY_FACEBOOK_COUNT,
	PAGE_ACTIVITY_LINKEDIN_COUNT,
	PAGE_ACTIVITY_YOUTUBE_COUNT,
	PAGE_ACTIVITY_INSTAGRAM_COUNT,
	PAGE_ACTIVITY_TWITTER_COUNT,
	PAGE_ACTIVITY_GOOGLE_COUNT,
	PAGE_ACTIVITY_YAHOO_COUNT,
	PAGE_ACTIVITY_OK_COUNT,
	PAGE_ACTIVITY_VK_COUNT,
	PAGE_ACTIVITY_WHATSAPP_COUNT,
	GENERAL_APPLICATION_START_COUNT,
	GENERAL_RECALIBRATION_COUNT,
	GENERAL_DRIFT_GRID_COUNT,
	GENERAL_URL_INPUT_COUNT,
	GENERAL_BOOKMARK_USAGE_COUNT,
	GENERAL_BOOKMARK_ADDING_COUNT,
	GENERAL_BOOKMARK_REMOVAL_COUNT,
	GENERAL_HISTORY_USAGE_COUNT,
	GENERAL_TAB_RELOADING_COUNT,
	GENERAL_TAB_CLOSING_COUNT,
	GENERAL_TAB_CREATION_COUNT,
	GENERAL_TAB_SWITCHING_COUNT,
	GENERAL_GO_BACK_USAGE_COUNT,
	GENERAL_GO_FORWARD_USAGE_COUNT,
	GENERAL_PAUSE_COUNT,
	GENERAL_UNPAUSE_COUNT,

	// TODO
	GENERAL_DASHBOARD_USAGE_COUNT,
};
enum class FirebaseStringKey	{ 
	TEST_STRING };
enum class FirebaseJSONKey		{ 
	PAGE_ACTIVITY_UNKNOWN,
	PAGE_ACTIVITY_NEWS,
	PAGE_ACTIVITY_SHOPPING,
	PAGE_ACTIVITY_EMAIL,
	PAGE_ACTIVITY_JOB,
	PAGE_ACTIVITY_ENTERTAINMENT,
	PAGE_ACTIVITY_FORUM,
	PAGE_ACTIVITY_HEALTH,
	PAGE_ACTIVITY_ELEARNING,
	PAGE_ACTIVITY_WIKIPEDIA,
	PAGE_ACTIVITY_FACEBOOK,
	PAGE_ACTIVITY_LINKEDIN,
	PAGE_ACTIVITY_YOUTUBE,
	PAGE_ACTIVITY_INSTAGRAM,
	PAGE_ACTIVITY_TWITTER,
	PAGE_ACTIVITY_GOOGLE,
	PAGE_ACTIVITY_YAHOO,
	PAGE_ACTIVITY_OK,
	PAGE_ACTIVITY_VK,
	PAGE_ACTIVITY_WHATSAPP,
	GENERAL_APPLICATION_START,
	GENERAL_RECALIBRATION,
	GENERAL_DRIFT_GRID,
	GENERAL_URL_INPUT,
	GENERAL_BOOKMARK_USAGE,
	GENERAL_BOOKMARK_ADDING,
	GENERAL_BOOKMARK_REMOVAL,
	GENERAL_HISTORY_USAGE,
	GENERAL_TAB_RELOADING,
	GENERAL_TAB_CLOSING,
	GENERAL_TAB_CREATION,
	GENERAL_TAB_SWITCHING,
	GENERAL_GO_BACK_USAGE,
	GENERAL_GO_FORWARD_USAGE,
	GENERAL_PAUSE,
	GENERAL_UNPAUSE,

	// Other
	SCORES_TOTAL,

	// TODO
	GENERAL_DASHBOARD_USAGE,
};

// Mapping from key to raw type
template<typename Type> struct FirebaseValue;
template<> struct FirebaseValue<FirebaseIntegerKey>	{ typedef int type; };
template<> struct FirebaseValue<FirebaseStringKey>	{ typedef std::string type; };
template<> struct FirebaseValue<FirebaseJSONKey>	{ typedef nlohmann::json type; };


// Firebase mailer class
class FirebaseMailer
{
private:

	// Mapping from key to address
	template<typename T>
	static std::string FirebaseAddress(T key)
	{	
		return "";
	}
	template<>
	static std::string FirebaseAddress<FirebaseIntegerKey>(FirebaseIntegerKey key)
	{
		switch (key)
		{

		// Page activity
		case FirebaseIntegerKey::PAGE_ACTIVITY_UNKNOWN_COUNT:
			return "pageActivity/_unknown/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_NEWS_COUNT:
			return "pageActivity/news/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_SHOPPING_COUNT:
			return "pageActivity/shopping/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_EMAIL_COUNT:
			return "pageActivity/email/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_JOB_COUNT:
			return "pageActivity/job/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_ENTERTAINMENT_COUNT:
			return "pageActivity/entertainment/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_FORUM_COUNT:
			return "pageActivity/forum/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_HEALTH_COUNT:
			return "pageActivity/health/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_ELEARNING_COUNT:
			return "pageActivity/elearning/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_WIKIPEDIA_COUNT:
			return "pageActivity/wikipedia/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_FACEBOOK_COUNT:
			return "pageActivity/facebook/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_LINKEDIN_COUNT:
			return "pageActivity/linkedin/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_YOUTUBE_COUNT:
			return "pageActivity/youtube/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_INSTAGRAM_COUNT:
			return "pageActivity/instagram/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_TWITTER_COUNT:
			return "pageActivity/twitter/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_GOOGLE_COUNT:
			return "pageActivity/google/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_YAHOO_COUNT:
			return "pageActivity/yahoo/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_OK_COUNT:
			return "pageActivity/ok/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_VK_COUNT:
			return "pageActivity/vk/sessionCount";
		case FirebaseIntegerKey::PAGE_ACTIVITY_WHATSAPP_COUNT:
			return "pageActivity/whatsapp/sessionCount";

		// General
		case FirebaseIntegerKey::GENERAL_APPLICATION_START_COUNT:
			return "general/start/count";
		case FirebaseIntegerKey::GENERAL_RECALIBRATION_COUNT:
			return "general/recalibration/count";
		case FirebaseIntegerKey::GENERAL_DRIFT_GRID_COUNT:
			return "general/driftGrid/count";
		case FirebaseIntegerKey::GENERAL_URL_INPUT_COUNT:
			return "general/urlInput/count";
		case FirebaseIntegerKey::GENERAL_BOOKMARK_USAGE_COUNT:
			return "general/bookmarkUsage/count";
		case FirebaseIntegerKey::GENERAL_BOOKMARK_ADDING_COUNT:
			return "general/bookmarkAdding/count";
		case FirebaseIntegerKey::GENERAL_BOOKMARK_REMOVAL_COUNT:
			return "general/bookmarkRemoval/count";
		case FirebaseIntegerKey::GENERAL_HISTORY_USAGE_COUNT:
			return "general/historyUsage/count";
		case FirebaseIntegerKey::GENERAL_TAB_RELOADING_COUNT:
			return "general/tabReloading/count";
		case FirebaseIntegerKey::GENERAL_TAB_CLOSING_COUNT:
			return "general/tabClosing/count";
		case FirebaseIntegerKey::GENERAL_TAB_CREATION_COUNT:
			return "general/tabCreation/count";
		case FirebaseIntegerKey::GENERAL_TAB_SWITCHING_COUNT:
			return "general/tabSwitching/count";
		case FirebaseIntegerKey::GENERAL_GO_BACK_USAGE_COUNT:
			return "general/goBackUsage/count";
		case FirebaseIntegerKey::GENERAL_GO_FORWARD_USAGE_COUNT:
			return "general/goForwardUsage/count";
		case FirebaseIntegerKey::GENERAL_PAUSE_COUNT:
			return "general/pause/count";
		case FirebaseIntegerKey::GENERAL_UNPAUSE_COUNT:
			return "general/unpause/count";
		case FirebaseIntegerKey::GENERAL_DASHBOARD_USAGE_COUNT:
			return "general/dashboardUsage/count";
		default: return "";
		}
	};
	template<>
	static std::string FirebaseAddress<FirebaseStringKey>(FirebaseStringKey key)
	{
		switch (key)
		{
		case FirebaseStringKey::TEST_STRING:
			return "testString";
		default: return "";
		}
	};
	template<>
	static std::string FirebaseAddress<FirebaseJSONKey>(FirebaseJSONKey key)
	{
		switch (key)
		{

		// Page activity
		case FirebaseJSONKey::PAGE_ACTIVITY_UNKNOWN:
			return "pageActivity/_unknown";
		case FirebaseJSONKey::PAGE_ACTIVITY_NEWS:
			return "pageActivity/news";
		case FirebaseJSONKey::PAGE_ACTIVITY_SHOPPING:
			return "pageActivity/shopping";
		case FirebaseJSONKey::PAGE_ACTIVITY_EMAIL:
			return "pageActivity/email";
		case FirebaseJSONKey::PAGE_ACTIVITY_JOB:
			return "pageActivity/job";
		case FirebaseJSONKey::PAGE_ACTIVITY_ENTERTAINMENT:
			return "pageActivity/entertainment";
		case FirebaseJSONKey::PAGE_ACTIVITY_FORUM:
			return "pageActivity/forum";
		case FirebaseJSONKey::PAGE_ACTIVITY_HEALTH:
			return "pageActivity/health";
		case FirebaseJSONKey::PAGE_ACTIVITY_ELEARNING:
			return "pageActivity/elearning";
		case FirebaseJSONKey::PAGE_ACTIVITY_WIKIPEDIA:
			return "pageActivity/wikipedia";
		case FirebaseJSONKey::PAGE_ACTIVITY_FACEBOOK:
			return "pageActivity/facebook";
		case FirebaseJSONKey::PAGE_ACTIVITY_LINKEDIN:
			return "pageActivity/linkedin";
		case FirebaseJSONKey::PAGE_ACTIVITY_YOUTUBE:
			return "pageActivity/youtube";
		case FirebaseJSONKey::PAGE_ACTIVITY_INSTAGRAM:
			return "pageActivity/instagram";
		case FirebaseJSONKey::PAGE_ACTIVITY_TWITTER:
			return "pageActivity/twitter";
		case FirebaseJSONKey::PAGE_ACTIVITY_GOOGLE:
			return "pageActivity/google";
		case FirebaseJSONKey::PAGE_ACTIVITY_YAHOO:
			return "pageActivity/yahoo";
		case FirebaseJSONKey::PAGE_ACTIVITY_OK:
			return "pageActivity/ok";
		case FirebaseJSONKey::PAGE_ACTIVITY_VK:
			return "pageActivity/vk";
		case FirebaseJSONKey::PAGE_ACTIVITY_WHATSAPP:
			return "pageActivity/whatsapp";

		// General
		case FirebaseJSONKey::GENERAL_APPLICATION_START:
			return "general/start";
		case FirebaseJSONKey::GENERAL_RECALIBRATION:
			return "general/recalibration";
		case FirebaseJSONKey::GENERAL_DRIFT_GRID:
			return "general/driftGrid";
		case FirebaseJSONKey::GENERAL_URL_INPUT:
			return "general/urlInput";
		case FirebaseJSONKey::GENERAL_BOOKMARK_USAGE:
			return "general/bookmarkUsage";
		case FirebaseJSONKey::GENERAL_BOOKMARK_ADDING:
			return "general/bookmarkAdding";
		case FirebaseJSONKey::GENERAL_BOOKMARK_REMOVAL:
			return "general/bookmarkRemoval";
		case FirebaseJSONKey::GENERAL_HISTORY_USAGE:
			return "general/historyUsage";
		case FirebaseJSONKey::GENERAL_TAB_RELOADING:
			return "general/tabReloading";
		case FirebaseJSONKey::GENERAL_TAB_CLOSING:
			return "general/tabClosing";
		case FirebaseJSONKey::GENERAL_TAB_CREATION:
			return "general/tabCreation";
		case FirebaseJSONKey::GENERAL_TAB_SWITCHING:
			return "general/tabSwitching";
		case FirebaseJSONKey::GENERAL_GO_BACK_USAGE:
			return "general/goBackUsage";
		case FirebaseJSONKey::GENERAL_GO_FORWARD_USAGE:
			return "general/goForwardUsage";
		case FirebaseJSONKey::GENERAL_PAUSE:
			return "general/pause";
		case FirebaseJSONKey::GENERAL_UNPAUSE:
			return "general/unpause";
		case FirebaseJSONKey::GENERAL_DASHBOARD_USAGE:
			return "general/dashboardUsage";

		// Other
		case FirebaseJSONKey::SCORES_TOTAL:
			return "scores/total";

		default: return "";
		}
	};

	// Address creation
	template<typename T>
	static std::string BuildFirebaseKey(T key, std::string uid)
	{
		return "users/" + uid + "/" + FirebaseAddress<T>(key);
	}

public:

	// Getter of static instance
	static FirebaseMailer& Instance()
	{
		static FirebaseMailer _instance; // threadsafe through C++11 standard
		return _instance;
	}

	// Destructor
	~FirebaseMailer() {
		_shouldStop = true;
		_conditionVariable.notify_all();
		_upThread->join();
	} // tell thread to stop

	// Continue mailer
	void Continue() { _paused = false; }

	// Pause mailer
	void Pause() { _paused = true; }

	// Available commands. Returns whether successful pushed back the command. If not, do not wait for the promise to be fulfilled!
	bool PushBack_Login		(std::string email, std::string password, bool useDriftMap, std::promise<std::string>* pPromise = nullptr); // promise delivers initial idToken value and sets internal start index
	bool PushBack_Transform	(FirebaseIntegerKey key, int delta, std::promise<int>* pPromise = nullptr); // promise delivers future database value
	bool PushBack_Maximum	(FirebaseIntegerKey key, int value, std::promise<int>* pPromise = nullptr); // promise delivers future database value
	bool PushBack_Put		(FirebaseIntegerKey key, int value, std::string subpath = "");
	bool PushBack_Put		(FirebaseStringKey key, std::string value, std::string subpath = "");
	bool PushBack_Put		(FirebaseJSONKey key, nlohmann::json value, std::string subpath = ""); // json return value might be empty in case of failure
	bool PushBack_Get		(FirebaseIntegerKey key, std::promise<int>* pPromise);
	bool PushBack_Get		(FirebaseStringKey key, std::promise<std::string>* pPromise);
	bool PushBack_Get		(FirebaseJSONKey key, std::promise<nlohmann::json>* pPromise);

	// Get id token (is empty before login or at failure)
	std::string GetIdToken() const;

	// Get start index (is -1 one at failure)
	int GetStartIndex() const;

	// Retrieve info about user award and wait for it
	Award GetUserAward()
	{
		// Retrieve score
		float score = 0.f;
		std::promise<nlohmann::json> scorePromise; auto scoreFuture = scorePromise.get_future(); // future provides score
		bool pushedBack = FirebaseMailer::Instance().PushBack_Get(FirebaseJSONKey::SCORES_TOTAL, &scorePromise);

		// Check whether it was returned
		if (pushedBack)
		{
			auto result = scoreFuture.get(); // get future (get may be only called once)
			if (!result.empty() && result.is_number()) // check whether there was really a score
			{
				score = result.get<float>();
			}
		}

		// Decide on award
		if (score <= 3.3f)
		{
			return Award::BRONZE;
		}
		else if (score <= 6.6f)
		{
			return Award::SILVER;
		}
		else
		{
			return Award::GOLD;
		}
	}

private:

	// Forward declaration
	class IdToken;

	// ### Delegate running in a thread ###
	class FirebaseInterface
	{
	public:

		// Constructor
		FirebaseInterface(IdToken* pIdToken, std::atomic<int>* pStartIndex) : _pIdToken(pIdToken), _pStartIndex(pStartIndex) {}

		// Log in. Return whether successful
		bool Login(std::string email, std::string password, bool useDriftMap, std::promise<std::string>* pPromise);

		// Simple put functionality. Replaces existing value if available, no ETag used
		template<typename T>
		void Put(T key, typename FirebaseValue<T>::type value, std::string subpath = ""); // delegates private put

		// Get
		template<typename T>
		void Get(T key, std::promise<typename FirebaseValue<T>::type>* pPromise); // delegates private get

		// Transform value
		void Transform(FirebaseIntegerKey key, int delta, std::promise<int>* pPromise = nullptr); // if nullptr, no future is set

		// Save maximum in database, either my value or the one in the database
		void Maximum(FirebaseIntegerKey key, int value, std::promise<int>* pPromise = nullptr); // if nullptr, no future is set

	private:

		// Struct for ETag and database value
		struct DBEntry
		{
			DBEntry() {};
			DBEntry(std::string ETag, nlohmann::json value) : ETag(ETag), value(value) {};
			std::string ETag = "";
			nlohmann::json value;
		};

		// Login via set email and password
		bool Login();

		// Relogin via refresh token. Returns whether successful
		bool Relogin();

		// Put single value in database. Uses ETag to check whether ok to put or working on outdated value. Return true if successful, otherwise do it another time, maybe with new value provided by this method
		template<typename T> // TODO: here on could get rid of templates and just work on JSON objects
		bool Put(T key, std::string ETag, typename FirebaseValue<T>::type value, std::string& rNewETag, typename FirebaseValue<T>::type& rNewValue, std::string subpath = ""); // fills newETag and newValue if ETag was outdated. Please try again with these values

		// Get of JSON structure by key. Returns empty structure if not available
		DBEntry Get(std::string key);

		// Apply function on value in database. Returns final value that is stored in database
		int Apply(FirebaseIntegerKey key, std::function<int(int)> function); // function parameter takes value from database on which function is applied. Its return value is then written to database

		// Constants
		const std::string _API_KEY = setup::FIREBASE_API_KEY;
		const std::string _URL = "https://" + setup::FIREBASE_PROJECT_ID + ".firebaseio.com";

		// Members
		IdToken* _pIdToken = nullptr; // set at construction
		std::atomic<int>* _pStartIndex = nullptr; // set at construction
		std::string _refreshToken = ""; // long living token for refreshing itself and idToken
		std::string _uid = "0"; // user identifier (initialized with something that indicates "broken")
		std::string _email = ""; // taken from login attempt
		std::string _password = ""; // taken from login attempt (guess this is bad design to store password, but simplies everything a lot)
	};
	// #################################

	// Typdef of command
	typedef const std::function<void(FirebaseInterface&)> Command;

	// Method that actually pushes back command (considers whether paused etc.)
	bool PushBackCommand(std::shared_ptr<Command> spCommand);

	// Private copy / assignment constructors
	FirebaseMailer(); // threadsafe as only called by Instance()
	FirebaseMailer(const FirebaseMailer&) {}
	FirebaseMailer& operator = (const FirebaseMailer &) { return *this; }

	// Pause indicator (if true, avoids pushing to command queue)
	std::atomic<bool> _paused = false; // atomic since could be accessed from multiple async threads

	// Start index of application run
	std::atomic<int> _startIndex = -1;

	// #### THREAD-RELATED MEMBERS ####

	// Id token may be readable from outside, but only set within FirebaseInterface thread
	class IdToken
	{
	public:
		void Set(std::string value)	{ std::lock_guard<std::mutex> lock(_lock); _value = value; }
		void Reset()				{ std::lock_guard<std::mutex> lock(_lock); _value = ""; }
		std::string Get() const		{ std::lock_guard<std::mutex> lock(_lock); return _value; }
		bool IsSet() const			{ std::lock_guard<std::mutex> lock(_lock); return !_value.empty(); }

	private:
		std::string _value = "";
		mutable std::mutex _lock; // mutable as can be even changed in const methods
	};

	// Threading (thread defined in constructor of FirebaseMailer)
	std::mutex _commandMutex; // mutex for access of _commandQueue (thread grabs all commands and works on them)
	std::condition_variable _conditionVariable; // used to wake up thread at available work
	std::deque<std::shared_ptr<Command> > _commandQueue; // shared function pointers that are executed sequentially within thread
	std::unique_ptr<std::thread> _upThread; // the thread itself
	std::atomic<bool> _shouldStop = false; // written by this, read by thread
	IdToken _idToken; // short living token for identification (indicator for being logged in!)

	// ################################
};

#endif // FIREBASEMAILER_H_