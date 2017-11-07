#pragma once

class AFX_EXT_CLASS CStopWatch
{
public:
	CStopWatch(void);
	~CStopWatch(void);

	void Start();
	float GetElapsedTime();
	
	bool IsRunning() { return m_bIsRunning;}
private:
	bool m_bIsRunning;

	LONGLONG m_llLastElapsedTick;
	LONGLONG m_llQPFTicksPerSecs;
	BOOL m_bUsingQPF;
};
