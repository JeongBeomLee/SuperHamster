#pragma once
class Timer
{
public:
	void Init()
	{
		::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));
	}

	void Update()
	{
		uint64 currentCount;
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

		_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
		_prevCount = currentCount;

		_frameCount++;
		_frameTime += _deltaTime;

		if (_frameTime > 1.f) {
			_fps = static_cast<uint32>(_frameCount / _frameTime);

			_frameTime = 0.f;
			_frameCount = 0;
		}
	}

	uint32 GetFps() { return _fps; }
	float GetDeltaTime() { return _deltaTime; }

private:
	uint64	_frequency = 0;
	uint64	_prevCount = 0;
	float	_deltaTime = 0.f;

private:
	uint32	_frameCount = 0;
	float	_frameTime = 0.f;
	uint32	_fps = 0;
};