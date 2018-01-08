/*
 * Copyright (C) 2017 University of Utah, derived from the RTXI open-source LaserController
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This module takes the input of Frequency and Duty cycle then outputs TTL on/off voltages accordingly.
 * For example the default frequency of 30 Hz with a 25% duty will have
 * (1000/30 * .25) = 8.3 ms on, then (33.3* .75) = 24.9 ms off, cycling.
 */

#include "LaserController.h"
#include <iostream>
#include <main_window.h>

extern "C" Plugin::Object*
createRTXIPlugin(void)
{
  return new LaserController();
}

/// The GUI labels for the needed inputs: frequency and dutyCyclePercentage.
static DefaultGUIModel::variable_t vars[] = {
  {
    "Frequency (Hz)", "From .3Hz to 3000 Hz. Out of bounds will adjust to max or min.",
    DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
  {
    "Duty Cycle (%)", "0 to 100. Out of bounds will adjust to max or min.", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

LaserController::LaserController(void)
  : DefaultGUIModel("TTL Signal Generator", ::vars, ::num_vars)
{
  setWhatsThis("<p><b>TTL Signal Generator:</b><br>QWhatsThis description.</p>");
  DefaultGUIModel::createGUI(vars,
                             num_vars); // this is required to create the GUI
  customizeGUI();
  initParameters();
  update(INIT); // this is optional, you may place initialization code directly
                // into the constructor
  refresh();    // this is required to update the GUI with parameter and state
                // values
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

LaserController::~LaserController(void){}


/// The real-time loop.
void LaserController::execute(void)
{
  // Where are we within a cycle? (elapsedTime modulos back to where we are in a cycle)
  elapsedTime = elapsedTime % cycleTime;

  if( (elapsedTime) >= dutyTimeWithinCycle){
    output(0) = 0; // Turn current off (0V)
  }else{
    output(0) = 5; // Turn current on (5V)
  }
  elapsedTime += period
  return;
}

/// Default values: frequency is 30 Hz so cycleTime = 1000/30 = 33.3 ms, dutyCyclePercentage = 25% = .25
void LaserController::initParameters(void)
{
  cycleTime = 1000/30;  //ms  (30 Hz)
  dutyCyclePercentage = 0.25; // = 25%
  elapsedTime = 0; // ms
  dutyTimeWithinCycle = cycleTime * dutyCyclePercentage;
}

void LaserController::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms

      setParameter("Frequency (Hz)", cycleTime);
      setParameter("Duty Cycle (%)", dutyCyclePercentage);

      //Adjust to proper units
      cycleTime = 1000/cycleTime; // ms/frequency
      dutyCyclePercentage /= 100; //The gui asks for %, we need decimal

      tetherDutyCycle(); //keep within 0.0->1.0
      tetherFrequency(); //keep within .3Hz -> 3kHz

      dutyTimeWithinCycle = cycleTime * dutyCyclePercentage;
      break;

    case MODIFY:
      cycleTime = 1000/ getParameter("Frequency (Hz)").toDouble(); //ms
      dutyCyclePercentage = getParameter("Duty Cycle (%)").toDouble();

      tetherDutyCycle(); //keep within 0.0->1.0
      tetherFrequency(); //keep within .3Hz -> 3kHz

      dutyTimeWithinCycle = cycleTime*dutyCyclePercentage; //ms
      break;

    case UNPAUSE:
      break;

    case PAUSE:
      output(0) = 0; //When paused, stop current
      break;

    case PERIOD:
      period = RT::System::getInstance()->getPeriod() * 1e-6; // ms
      break;

    default:
      break;
  }
}

/// Keep dutyCyclePercentage within 0% to 100%
private void tetherDutyCycle(){
  if(dutyCyclePercentage >1){
    dutyCyclePercentage = 1;
  } if (dutyCyclePercentage < 0){
    dutyCyclePercentage = 0;
  }
}

/// Keep the 'frequency' between .3 -> 3000 Hz, by keeping the cycle time (ms) within those bounds.
/// This should be done before computing the dutyTimeWithinCycle.
private void tetherFrequency(){
  // Frequency is within .3 -> 3000 Hz, so cycle is between 1000/.3 -> 1000/3000
  if(cycleTime >= (1000/.3)){
    cycleTime = 1000/.3;
  }else if(cycleTime <= 1000/3000){
    cycleTime = 1000/3000;
  }
}

// J- so far as I know I don't need a custom GUI
void
LaserController::customizeGUI(void)
{
  // QGridLayout* customlayout = DefaultGUIModel::getLayout();
  //
  // QGroupBox* button_group = new QGroupBox;
  //
  // QPushButton* abutton = new QPushButton("Button A");
  // QPushButton* bbutton = new QPushButton("Button B");
  // QHBoxLayout* button_layout = new QHBoxLayout;
  // button_group->setLayout(button_layout);
  // button_layout->addWidget(abutton);
  // button_layout->addWidget(bbutton);
  // QObject::connect(abutton, SIGNAL(clicked()), this, SLOT(aBttn_event()));
  // QObject::connect(bbutton, SIGNAL(clicked()), this, SLOT(bBttn_event()));
  //
  // customlayout->addWidget(button_group, 0, 0);
  // setLayout(customlayout);
}

// // functions designated as Qt slots are implemented as regular C++ functions
// void
// LaserController::aBttn_event(void)
// {
// }
//
// void
// LaserController::bBttn_event(void)
// {
// }
