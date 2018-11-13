//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Structure to hold a social record, for example a Facebook visit.

#ifndef SOCIALRECORD_H_
#define SOCIALRECORD_H_

#include "src/Utils/Helper.h"
#include "src/Singletons/FirebaseMailer.h"
#include "submodules/json/src/json.hpp"
#include <string>
#include <chrono>

// List of social platforms (mind the order)
enum class SocialPlatform
{
	Unknown, News, Shopping, Email, Job, Entertainment, Forum, Health, ELearning, Wikipedia, Facebook, Linkedin, YouTube, Instagram, Twitter, Google, Yahoo, Ok, Vk, Whatsapp // Email does not include GMail!
};

// List of identifying domain parts for social platforms (mind the order)
static const std::map < SocialPlatform, std::vector<std::string> > SocialIdentifiers =
{
	{ SocialPlatform::News,		{ "yahoo.com", "reddit.com", "msn.com", "theguardian.com"," nba.com", "ekathimerini.com", "protothema.gr", "sport24.gr",
								"gazzetta.gr", "protothema.gr", "newsit.gr", "iefimerida.gr", "news247.gr", "newsbomb.gr", "sport24.gr", "newsbeast.gr", "zougla.gr",
								"contra.gr", "lifo.gr", "pronews.gr", "in.gr", "taxheaven.gr", "protothema.gr", "kathimerini.gr", "tovima.gr", "iefimerida.gr", "diaforetiko.gr",
								"protothema.gr", "novasports.gr", "ynet.co.il", "walla.co.il", "mako.co.il", "sport5.co.il", "haaretz.co.il", "globes.co.il", "one.co.il", "calcalist.co.il",
								"tapuz.co.il", "themarker.com" } },
	{ SocialPlatform::Shopping,	{ "ebay.com", "amazon.com", "amazon.co.uk", "skroutz.gr", "car.gr", "xe.gr", "mawdoo3.com", "aliexpress.com", "yad2.co.il",
								  "zap.co.il", "next.co.il", "ksp.co.il"} },
	{ SocialPlatform::Email,	{ "mail.ru", "live.com", "outlook.live.com", "mail.google.com", "mail.yahoo.com", "mail.walla.co.il" } },
	{ SocialPlatform::Job,		{ "alljobs.co.il", "drushim.co.il", "jobmaster.co.il", "jobnet.co.il" } },
	{ SocialPlatform::Entertainment,	{ "netflix.com", "gamato.gr", "dailybuzz.co.il", "sdarot.click", "tvil.me" } },
	{ SocialPlatform::Forum,	{ "mdahellas.gr", "artimeleia.gr", "zap.co.il", "carsforum.co.il", "atraf.co.il", "yad2.co.il" } },
	{ SocialPlatform::Health,	{ "iherb.com", "parkinsonofficial.gr", "tlvmed.co.il", "maccabi4u.co.il", "clalit.org.il", "healthy.walla.co.il" } },
	{ SocialPlatform::ELearning,{ "coursera.com", "coursera.org", "elearn.elke.uoa.gr", "cet.ac.il", "education.gov.il", "openu.ac.il", "huji.ac.il", "tau.ac.il" } },
	{ SocialPlatform::Wikipedia,{ "wikipedia.org" } },
	{ SocialPlatform::Facebook,	{ "facebook.com", "facebook.co.il" } },
	{ SocialPlatform::Linkedin,	{ "linkedin.com" } },
	{ SocialPlatform::YouTube,	{ "youtube.com", "youtu.be" } },
	{ SocialPlatform::Instagram,{ "instagram.com" } },
	{ SocialPlatform::Twitter,	{ "twitter.com", "twitter.co.il" } },
	{ SocialPlatform::Google,	{ "google.com", "goog.le", "google.de", "google.gr", "google.co.il", "google.ps", "google.ru" } },
	{ SocialPlatform::Yahoo,	{ "yahoo.com" } },
	{ SocialPlatform::Ok,		{ "ok.ru" } },
	{ SocialPlatform::Vk,		{ "vk.com" } },
	{ SocialPlatform::Whatsapp,	{ "whatsapp.com" } }
};

// Mapping from platform to database keys (pair of count and record key)
static const std::map <SocialPlatform, std::pair<FirebaseIntegerKey, FirebaseJSONKey> > SocialFirebaseKeys =
{
	{ SocialPlatform::Unknown,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_UNKNOWN_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_UNKNOWN) },
	{ SocialPlatform::News,		std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_NEWS_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_NEWS) },
	{ SocialPlatform::Shopping,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_SHOPPING_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_SHOPPING) },
	{ SocialPlatform::Email,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_EMAIL_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_EMAIL) },
	{ SocialPlatform::Job,		std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_JOB_COUNT,			FirebaseJSONKey::PAGE_ACTIVITY_JOB) },
	{ SocialPlatform::Entertainment,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_ENTERTAINMENT_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_ENTERTAINMENT) },
	{ SocialPlatform::Forum,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_FORUM_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_FORUM) },
	{ SocialPlatform::Health,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_HEALTH_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_HEALTH) },
	{ SocialPlatform::ELearning,std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_ELEARNING_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_ELEARNING) },
	{ SocialPlatform::Wikipedia,std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_WIKIPEDIA_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_WIKIPEDIA) },
	{ SocialPlatform::Facebook,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_FACEBOOK_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_FACEBOOK) },
	{ SocialPlatform::Linkedin,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_LINKEDIN_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_LINKEDIN) },
	{ SocialPlatform::YouTube,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_YOUTUBE_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_YOUTUBE) },
	{ SocialPlatform::Instagram,std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_INSTAGRAM_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_INSTAGRAM) },
	{ SocialPlatform::Twitter,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_TWITTER_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_TWITTER) },
	{ SocialPlatform::Google,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_GOOGLE_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_GOOGLE) },
	{ SocialPlatform::Yahoo,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_YAHOO_COUNT,		FirebaseJSONKey::PAGE_ACTIVITY_YAHOO) },
	{ SocialPlatform::Ok,		std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_OK_COUNT,			FirebaseJSONKey::PAGE_ACTIVITY_OK) },
	{ SocialPlatform::Vk,		std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_VK_COUNT,			FirebaseJSONKey::PAGE_ACTIVITY_VK) },
	{ SocialPlatform::Whatsapp,	std::make_pair(FirebaseIntegerKey::PAGE_ACTIVITY_WHATSAPP_COUNT,	FirebaseJSONKey::PAGE_ACTIVITY_WHATSAPP) },
};

// Class of social record
class SocialRecord
{
public:

	// Extract domain of URL
	static std::string ExtractDomain(const std::string& rURL)
	{
		return ShortenURL(rURL);
	}

	// Static function to classify current URL. Does return unknown when social platform cannot be determined
	static SocialPlatform ClassifyURL(std::string URL)
	{
		// Extract domain
		const std::string domain = ExtractDomain(URL);

		// Go over social identifiers and search for them in the domain
		SocialPlatform platform = SocialPlatform::Unknown;
		for (const auto& rPlatform : SocialIdentifiers) // go over platforms
		{
			for (const auto& rIdentifier : rPlatform.second) // go over identifiers
			{
				if (domain.find(rIdentifier) != std::string::npos) // search for identifier
				{
					platform = rPlatform.first; // found the platform
					return platform; // instead of break
				}
			}
		}

		// Return result
		return platform;
	}

	// Constructor
	SocialRecord(std::string domain, SocialPlatform platform, int startIndex);

	// Virtual destructor
	virtual ~SocialRecord();

	// Start record
	void StartAndAddPage(std::string URL, std::string keywords);

	// End record
	void End();

	// Persist record (sends it to Firebase)
	void Persist();

	// Add time in foreground
	void AddTimeInForeground(float time);

	// Add time in foreground and user is active
	void AddTimeActiveUser(float time);

	// Add time in foreground and user uses emulation by mouse
	void AddTimeEmulatedInput(float time);

	// Add scrolling delta
	void AddManualScrollingDelta(float delta);
	void AddAutomaticScrollingDelta(float delta);

	// Add click
	void AddClick(std::string tag, std::string id, float x, float y);

	// Add text input
	void AddTextInput(std::string tag, std::string id, int charCount, int charDistance, float x, float y, float duration);

	// Add page
	void AddPage(std::string URL, std::string keywords);

	// Convert to JSON for storing in database
	nlohmann::json ToJSON() const;

	// Get platform
	SocialPlatform GetPlatform() const { return _platform; }

	// Get domain
	std::string GetDomain() const { return _domain; }

private:

	// Struct for click record
	struct Click
	{
		// Constructor
		Click(std::string tag, std::string id, float x, float y, double time) : tag(tag), id(id), x(x), y(y), time(time) {}

		// Fields
		const std::string tag;
		const std::string id;
		const float x;
		const float y;
		const double time;
	};

	// Struct for text input record
	struct TextInput
	{
		// Constructor
		TextInput(std::string tag, std::string id, int charCount, int charDistance, float x, float y, double time, float duration) : tag(tag), id(id), charCount(charCount), charDistance(charDistance), x(x), y(y), time(time), duration(duration) {}

		// Fields
		const std::string tag;
		const std::string id;
		const int charCount = 0;
		const int charDistance = 0;
		const float x;
		const float y;
		const double time;
		const float duration;
	};

	// Struct for page record
	struct Page
	{
		// Constructor
		Page(std::string URL, std::string keywords) : URL(URL), keywords(keywords) {}

		// Fields
		const std::string URL;
		const std::string keywords;
		double duration = 0.0;
		double durationInForeground = 0.0;
		double durationUserActive = 0.0; // and tab in foreground, input either by eye tracker and gaze detected or mouse
		double durationEmulatedInput = 0.0; // duration of emulated input while in foreground
		double manualScrollAmount = 0.0;
		double automaticScrollAmount = 0.0;
		std::vector<Click> clicks;
		std::vector<TextInput> textInputs;
	};

	// Get numbering with preceding zeros
	std::string PrecedeZeros(const std::string& rInput, const int digitCount) const;

	// Members
	const SocialPlatform _platform = SocialPlatform::Unknown;
	const std::string _domain;
	bool _writeable = false;
	std::chrono::time_point<std::chrono::system_clock> _startTime;
	std::chrono::time_point<std::chrono::system_clock> _endTime;
	std::chrono::time_point<std::chrono::system_clock> _lastAddPage; // used for calculating duration of page
	std::string _startDate;
	std::string _endDate;
	std::string _startTimestamp;
	std::string _endTimestamp;
	double _totalDurationInForeground = 0.0;
	double _totalDurationUserActive = 0.0; // and tab in foreground
	double _totalDurationEmulatedInput = 0.0;
	std::vector<Page> _pages; // should have at least one element and current one is at back
	int _startIndex = -1;
};

#endif // SOCIALRECORD_H_