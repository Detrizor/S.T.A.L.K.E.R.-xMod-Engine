#pragma once

class CFireDispertionController
{
private:
	float start_disp;
	float end_disp;
	float start_time;
	float current_disp;

public:
	static float crosshair_inertion;

	CFireDispertionController		();
	void	SetDispertion			(float const new_disp);
	float	GetCurrentDispertion	() const;
	void	Update					();
};
