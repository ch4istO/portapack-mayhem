/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UI_WHIPCALC_H__
#define __UI_WHIPCALC_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_receiver.hpp"
#include "ui_navigation.hpp"
#include "string_format.hpp"
#include <vector>

namespace ui {

class WhipCalcView : public View {
public:
	WhipCalcView(NavigationView& nav);
	
	void focus() override;
	
	std::string title() const override { return "Antenna length"; };

private:
	const double speed_of_light_mps = 299792458.0;		// m/s
	const double speed_of_light_fps = 983571087.90472;	// feet/s
	
	const std::string frac_str[4] = { "", " 1/4", " 1/2", " 3/4" };

	struct antenna_entry {
		std::string label { };
		std::vector <uint16_t> elements { };
	};

	std::vector<antenna_entry> antenna_db { };

	void update_result();

	uint16_t string_to_number(std::string);
	void txtline_process(std::string&);
	void antenna_Default();

	Labels labels {
		{ { 5 * 8, 1 * 16 }, "Loaded:", Color::light_grey() },
		{ { 2 * 8, 2 * 16 }, "Frequency:", Color::light_grey() },
		{ { 7 * 8, 3 * 16 }, "Wave:", Color::light_grey() },
		{ { 5 * 8, 4 * 16 }, "Metric:", Color::light_grey() },
		{ { 3 * 8, 5 * 16 }, "Imperial:", Color::light_grey() }
	};

	Text antennas_on_memory {
		{ 13 * 8, 1 * 16, 2 * 16, 16 },
		"-"
	};

	FrequencyField field_frequency {
		{ 13 * 8, 2 * 16 },
	};

	OptionsField options_type {
		{ 13 * 8, 3 * 16 },
		7,
		{
			{ "Full", 8 },
			{ "Half", 4 },
			{ "Quarter", 2 },
			{ "3/4", 6 },
			{ "1/8", 1 },
			{ "3/8", 3 },
			{ "5/8", 5 },
			{ "7/8", 7 }
		}
	};
	
	Text text_result_metric {
		{ 13 * 8, 4 * 16, 10 * 16, 16 },
		"-"
	};
	Text text_result_imperial {
		{ 13 * 8, 5 * 16, 10 * 16, 16 },
		"-"
	};

	Console console {
		{ 0, 7 * 16, 240, 144 }		//Allows to show up to 9 antennas
	};
	
	Button button_exit {
		{ 72, 17 * 16, 96, 32 },
		"Exit"
	};
};

} /* namespace ui */

#endif/*__UI_WHIPCALC__*/
