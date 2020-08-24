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

#include "ui_whipcalc.hpp"

#include "ch.h"
#include "portapack.hpp"
#include "event_m0.hpp"

#include <cstring>

using namespace portapack;

namespace ui {

void WhipCalcView::focus() {
	field_frequency.focus();
}

void WhipCalcView::update_result() {
	if (field_frequency.value() > 0)
	{
		double length, calclength, divider;
		divider = ((double)options_type.selected_index_value() / 8.0);

		length = (speed_of_light_mps / (double)field_frequency.value()) * divider; 	// Metric
		auto m = to_string_dec_int((int)length, 2);									//m
		calclength = get_decimals(length,100);										//cm
		auto cm = to_string_dec_int(int(calclength), 2);
		auto mm = to_string_dec_int(int(get_decimals(calclength,10,true)), 1);		//mm
		text_result_metric.set(m + "m " + cm + "." + mm + "cm");

		uint8_t antennas_shown = 0;
		console.clear(true);
		length *= 1000;		//Get length in mm needed to extend the antenna
		for (antenna_entry antenna : antenna_db) {	//go thru all antennas available
			if (length >= antenna.elements.front() && length <= antenna.elements.back()) //This antenna is OK
			{
				uint16_t element,refined_quarter=0;
				for(element=0; element < antenna.elements.size();element++) {
					if (length == antenna.elements[element]) 			//Exact element in length
					{	
						element++;	//Real element is +1  (zero based vector)
						break; 	//Done with this ant
					} 
					else if (length < antenna.elements[element]) 
					{
						double remain, this_element, quarter = 0;
						remain = length - antenna.elements[element-1]; 	//mm needed from this element to reach length
						this_element=antenna.elements[element] - antenna.elements[element -1];	//total mm on this element
						quarter = (remain * 4) / this_element;	//havoc & portack ended on this int(quarter) resolution.
						if (quarter - int(quarter) > 0.5) {	//rounding gave a measure closer to next quarter
							refined_quarter=int(quarter) + 1;
							if(refined_quarter == 4) {		//rounding gave a measure closer to next element
								refined_quarter = 0;
								element++;
							}
						} else {
							refined_quarter=int(quarter);
						}
						break;	//Done with this ant
					}
				}
				if (++antennas_shown == 9) {
					console.write(" and more ...");
					break;
				} 
				console.write(antenna.label + " " + to_string_dec_int(element,1) + frac_str[refined_quarter] + " elements\n");
			}
		}

		length = (speed_of_light_fps / (double)field_frequency.value()) * divider;	// Imperial
		auto feet = to_string_dec_int(int(length), 3);								//feet
		calclength = get_decimals(length,12);										//inches
		auto inch = to_string_dec_int(int(calclength), 2);
		auto inch_c = to_string_dec_int(int(get_decimals(calclength,10,true)), 1);	//inch decimal
		text_result_imperial.set(feet + "ft " + inch + "." + inch_c + "in");
	}
	else {		//freq. is zero
		text_result_metric.set("-");
		text_result_imperial.set("-");
	}
}

WhipCalcView::WhipCalcView(NavigationView& nav) {
	add_children({
		&labels,
		&antennas_on_memory,
		&field_frequency,
		&options_type,
		&text_result_metric,
		&text_result_imperial,
		&console,
		&button_exit
	});

	File antennas_file; 		//LOAD /WHIPCALC/ANTENNAS.TXT from microSD
	auto result = antennas_file.open("WHIPCALC/ANTENNAS.TXT");
	antenna_db.clear();			//Start with fresh db
	if (result.is_valid()) {
		antenna_Default(); 		//There is no txt, store a default ant500
	} else {
		std::string line;		//There is a txt file
		char one_char[1];		//Read it char by char
		for (size_t pointer=0; pointer < antennas_file.size();pointer++) {
			antennas_file.seek(pointer);
			antennas_file.read(one_char, 1);
			if ((int)one_char[0] > 31) {			//ascii space upwards
				line += one_char[0];				//Add it to the textline
			}
			else if (one_char[0] == '\n') {			//New Line
				txtline_process(line);				//make sense of this textline
				line.clear();						//Ready for next textline
			} 
		}
		if (line.length() > 0) txtline_process(line);	//Last line had no newline at end ?
		if (!antenna_db.size()) antenna_Default();		//no antenna on txt, use default
	}
	antennas_on_memory.set(to_string_dec_int(antenna_db.size(),2) + " antennas");	//tell user

	options_type.on_change = [this](size_t, OptionsField::value_t) {
		this->update_result();
	};
	options_type.set_selected_index(2);		// Quarter wave

	field_frequency.set_value(transmitter_model.tuning_frequency());
	field_frequency.set_step(1000000);		// 1Mhz step
	field_frequency.on_change = [this](rf::Frequency) {
		this->update_result();
	};
	field_frequency.on_edit = [this, &nav]() {
		// TODO: Provide separate modal method/scheme?
		auto new_view = nav.push<FrequencyKeypadView>(transmitter_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			this->update_result();
			this->field_frequency.set_value(f);
		};
	};
	
	button_exit.on_select = [this, &nav](Button&) {
		nav.pop();
	};
	update_result();
}

void ui::WhipCalcView::txtline_process(std::string& line) {
	if (line.find("#") != std::string::npos) return;	//Line is just a comment
	size_t previous = 0;
	uint16_t value = 0;
	antenna_entry new_antenna;
	size_t current = line.find(",");
	while (current != std::string::npos) {
		if (!previous) {
			new_antenna.label.assign(line,0,current);	//antenna label
		} else {
			value = std::stoi(line.substr(previous,current - previous));
			if (!value) return; 						//No element length? abort antenna
			new_antenna.elements.push_back(value);		//Store this new element
		}
		previous = current + 1;
		current = line.find(",",previous);				//Search for next delimiter
	}
	if (!previous) return;								//Not even a label ? drop this antenna!
	value = std::stoi(line.substr(previous,current - previous)); //Last element
	if (!value) return;
	new_antenna.elements.push_back(value);
	antenna_db.push_back(new_antenna); 					//Add this antenna
}

void ui::WhipCalcView::antenna_Default() {
	antenna_db.push_back({"ANT500",{ 185, 315, 450, 586, 724, 862} }); //store a default ant500
}

}
