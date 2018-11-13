//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Filter.h"

#include "src/Global.h"
#include "src/Setup.h"
#include <algorithm>

Filter::Filter() :
	_spSamples(SampleQueue(new std::deque<SampleData>))
{
	// Nothing to do
}

Filter::~Filter()
{
	// For the sake of the C++ standard
}

void Filter::Update(const SampleQueue spSamples, float samplerate)
{
	// Only work with non-empty sample queue
	if (!spSamples->empty())
	{
		// Update timestamp
		_timestamp = spSamples->back().timestamp; // should be newest sample
		_timestampSetOnce = true;
	}

	// Copy samples over to member
	_spSamples->insert(_spSamples->end(),
		spSamples->begin(),
		spSamples->end());
	
	// Delete front of queue to match maximum allowed queue length (delete oldest samples)
	int size = (int)_spSamples->size(); // sample queue size
	int overlap = size - setup::FILTER_MEMORY_SIZE;
	for (int i = 0; i < overlap; i++)
	{
		_spSamples->pop_front();
	}

	// Apply filtering to retrieve current filtered gaze coordinate and other information
	ApplyFilter(_spSamples, _gazeX, _gazeY, _fixationDuration, samplerate);

	// Work on custom transformations
	for (auto& rCustomTransformation : _customTransformations)
	{
		// Make reference on custom transformation within map
		auto& rTrans = rCustomTransformation.second;

		// Copy current samples and apply current transformation
		std::deque<SampleData> tmpQueue;
		tmpQueue.insert(tmpQueue.end(),
			spSamples->begin(),
			spSamples->end());
		for (auto& rSample : tmpQueue)
		{
			rTrans.transformation(rSample.x, rSample.y);
		}

		// Move samples to queue
		rTrans.queue->insert(rTrans.queue->end(),
			std::make_move_iterator(tmpQueue.begin()),
			std::make_move_iterator(tmpQueue.end()));

		// Delete front of queue to match maximum allowed queue length (delete oldest samples)
		size = (int)rTrans.queue->size(); // sample queue size
		overlap = size - setup::FILTER_MEMORY_SIZE;
		for (int i = 0; i < overlap; i++)
		{
			rTrans.queue->pop_front();
		}

		// Apply filtering
		float fixationDuration = 0; // not used
		ApplyFilter(
			rTrans.queue,
			rTrans.gazeX,
			rTrans.gazeY,
			fixationDuration,
			samplerate);
	}
}

double Filter::GetRawGazeX() const
{
	if (!_spSamples->empty())
	{
		return _spSamples->back().x;
	}
	else
	{
		return -1;
	}
}

double Filter::GetRawGazeY() const
{
	if (!_spSamples->empty())
	{
		return _spSamples->back().y;
	}
	else
	{
		return -1;
	}
}

double Filter::GetFilteredGazeX() const
{
	return _gazeX;
}

double Filter::GetFilteredGazeY() const
{
	return _gazeY;
}

float Filter::GetFixationDuration() const
{
	return _fixationDuration;
}

double Filter::GetAge() const
{
	if (_timestampSetOnce)
	{
		return std::min(
			FILTER_MAXIMUM_SAMPLE_AGE,
			((double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - _timestamp).count() / 1000.0));
	}
	else
	{
		return FILTER_MAXIMUM_SAMPLE_AGE;
	}
}

bool Filter::IsTimestampSetOnce() const
{
	return _timestampSetOnce;
}

bool Filter::RegisterCustomTransformation(std::string name, FilterTransformation transformation)
{
	// Check whether custom transformation already exists
	auto it = _customTransformations.find(name);
	if (it == _customTransformations.end())
	{
		// Create and fill structure
		CustomTransformation trans; // create struct
		trans.transformation = transformation; // set transformation
		trans.queue = SampleQueue(new std::deque<SampleData>(*_spSamples.get())); // deep copy of sample data

		// Apply transformation on already retrieved samples
		for (auto& rSample : *trans.queue.get())
		{
			trans.transformation(rSample.x, rSample.y);
		}

		// Insert this custom transformation
		_customTransformations.insert(std::make_pair(
			name,
			trans
		));
		return true;
	}
	return false;
}

bool Filter::ChangeCustomTransformation(std::string name, FilterTransformation transformation)
{
	auto it = _customTransformations.find(name);
	if (it != _customTransformations.end())
	{
		it->second.transformation = transformation;
		return true;
	}
	return false;
}

bool Filter::UnregisterCustomTransformation(std::string name)
{
	// Check whether custom transformation exists
	auto it = _customTransformations.find(name);
	if (it != _customTransformations.end())
	{
		_customTransformations.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}

double Filter::GetFilteredGazeX(std::string name) const
{
	return _customTransformations.at(name).gazeX;
}

double Filter::GetFilteredGazeY(std::string name) const
{
	return _customTransformations.at(name).gazeY;
}