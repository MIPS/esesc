/*****************************************************************************
 *                                McPAT
 *                      SOFTWARE LICENSE AGREEMENT
 *            Copyright 2009 Hewlett-Packard Development Company, L.P.
 *                          All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Any User of the software ("User"), by accessing and using it, agrees to the
 * terms and conditions set forth herein, and hereby grants back to Hewlett-
 * Packard Development Company, L.P. and its affiliated companies ("HP") a
 * non-exclusive, unrestricted, royalty-free right and license to copy,
 * modify, distribute copies, create derivate works and publicly display and
 * use, any changes, modifications, enhancements or extensions made to the
 * software by User, including but not limited to those affording
 * compatibility with other hardware or software, but excluding pre-existing
 * software applications that may incorporate the software.  User further
 * agrees to use its best efforts to inform HP of any such changes,
 * modifications, enhancements or extensions.
 *
 * Correspondence should be provided to HP at:
 *
 * Director of Intellectual Property Licensing
 * Office of Strategy and Technology
 * Hewlett-Packard Company
 * 1501 Page Mill Road
 * Palo Alto, California  94304
 *
 * The software may be further distributed by User (but not offered for
 * sale or transferred for compensation) to third parties, under the
 * condition that such third parties agree to abide by the terms and
 * conditions of this license.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITH ANY AND ALL ERRORS AND DEFECTS
 * AND USER ACKNOWLEDGES THAT THE SOFTWARE MAY CONTAIN ERRORS AND DEFECTS.
 * HP DISCLAIMS ALL WARRANTIES WITH REGARD TO THE SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL
 * HP BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 *
 ***************************************************************************/

#include "wire.h"

Wire::Wire(enum Wire_type wire_model, double wl, int n, double w_s, double s_s, enum Wire_placement wp, double resistivity,
           TechnologyParameter::DeviceType *dt)
    : wt(wire_model)
    , wire_length(wl * 1e-6)
    , nsense(n)
    , w_scale(w_s)
    , s_scale(s_s)
    , resistivity(resistivity)
    , deviceType(dt) {
  wire_placement = wp;
  min_w_pmos     = deviceType->n_to_p_eff_curr_drv_ratio * g_tp.min_w_nmos_;
  in_rise_time   = 0;
  out_rise_time  = 0;
  calculate_wire_stats();
  // change everything back to seconds, microns, and Joules
  repeater_spacing *= 1e6;
  wire_length *= 1e6;
  wire_width *= 1e6;
  wire_spacing *= 1e6;
  assert(wire_length > 0);
  assert(power.readOp.dynamic > 0);
  assert(power.readOp.leakage > 0);
  assert(power.readOp.gate_leakage > 0);
}

Wire::Wire(double w_s, double s_s, enum Wire_placement wp, double resis, TechnologyParameter::DeviceType *dt) {
  w_scale        = w_s;
  s_scale        = s_s;
  deviceType     = dt;
  wire_placement = wp;
  resistivity    = resis;
  min_w_pmos     = deviceType->n_to_p_eff_curr_drv_ratio * g_tp.min_w_nmos_;
  in_rise_time   = 0;
  out_rise_time  = 0;

  switch(wire_placement) {
  case outside_mat:
    wire_width = g_tp.wire_outside_mat.pitch;
    break;
  case inside_mat:
    wire_width = g_tp.wire_inside_mat.pitch;
    break;
  default:
    wire_width = g_tp.wire_local.pitch;
    break;
  }

  wire_spacing = wire_width;

  wire_width *= (w_scale * 1e-6 / 2) /* (m) */;
  wire_spacing *= (s_scale * 1e-6 / 2) /* (m) */;

  init_wire();

  assert(power.readOp.dynamic > 0);
  assert(power.readOp.leakage > 0);
  assert(power.readOp.gate_leakage > 0);
}

Wire::~Wire() {
}

// the following values are for peripheral global technology
// specified in the input config file
Component Wire::global;
Component Wire::global_5;
Component Wire::global_10;
Component Wire::global_20;
Component Wire::global_30;

void Wire::calculate_wire_stats() {

  if(wire_placement == outside_mat) {
    wire_width = g_tp.wire_outside_mat.pitch;
  } else if(wire_placement == inside_mat) {
    wire_width = g_tp.wire_inside_mat.pitch;
  } else {
    wire_width = g_tp.wire_local.pitch;
  }

  wire_spacing = wire_width;

  wire_width *= (w_scale * 1e-6 / 2) /* (m) */;
  wire_spacing *= (s_scale * 1e-6 / 2) /* (m) */;

  if(wt != Low_swing) {

    delay_optimal_wire();

    if(wt == Global_5) {
      delay                     = global_5.delay * wire_length;
      power.readOp.dynamic      = global_5.power.readOp.dynamic * wire_length;
      power.readOp.leakage      = global_5.power.readOp.leakage * wire_length;
      power.readOp.gate_leakage = global_5.power.readOp.gate_leakage * wire_length;
      repeater_spacing          = global_5.area.w;
      repeater_size             = global_5.area.h;
      area.set_area((wire_length / repeater_spacing) *
                    compute_gate_area(INV, 1, min_w_pmos * repeater_size, g_tp.min_w_nmos_ * repeater_size, g_tp.cell_h_def));
    } else if(wt == Global_10) {
      delay                     = global_10.delay * wire_length;
      power.readOp.dynamic      = global_10.power.readOp.dynamic * wire_length;
      power.readOp.leakage      = global_10.power.readOp.leakage * wire_length;
      power.readOp.gate_leakage = global_10.power.readOp.gate_leakage * wire_length;
      repeater_spacing          = global_10.area.w;
      repeater_size             = global_10.area.h;
      area.set_area((wire_length / repeater_spacing) *
                    compute_gate_area(INV, 1, min_w_pmos * repeater_size, g_tp.min_w_nmos_ * repeater_size, g_tp.cell_h_def));
    } else if(wt == Global_20) {
      delay                     = global_20.delay * wire_length;
      power.readOp.dynamic      = global_20.power.readOp.dynamic * wire_length;
      power.readOp.leakage      = global_20.power.readOp.leakage * wire_length;
      power.readOp.gate_leakage = global_20.power.readOp.gate_leakage * wire_length;
      repeater_spacing          = global_20.area.w;
      repeater_size             = global_20.area.h;
      area.set_area((wire_length / repeater_spacing) *
                    compute_gate_area(INV, 1, min_w_pmos * repeater_size, g_tp.min_w_nmos_ * repeater_size, g_tp.cell_h_def));
    } else if(wt == Global_30) {
      delay                     = global_30.delay * wire_length;
      power.readOp.dynamic      = global_30.power.readOp.dynamic * wire_length;
      power.readOp.leakage      = global_30.power.readOp.leakage * wire_length;
      power.readOp.gate_leakage = global_30.power.readOp.gate_leakage * wire_length;
      repeater_spacing          = global_30.area.w;
      repeater_size             = global_30.area.h;
      area.set_area((wire_length / repeater_spacing) *
                    compute_gate_area(INV, 1, min_w_pmos * repeater_size, g_tp.min_w_nmos_ * repeater_size, g_tp.cell_h_def));
    }
    out_rise_time = delay * repeater_spacing / deviceType->Vth;
  } else if(wt == Low_swing) {
    low_swing_model();
    repeater_spacing = wire_length;
    repeater_size    = 1;
  } else {
    assert(0);
  }
}

/*
 * The fall time of an input signal to the first stage of a circuit is
 * assumed to be same as the fall time of the output signal of two
 * inverters connected in series (refer: CACTI 1 Technical report,
 * section 6.1.3)
 */
double Wire::signal_fall_time() {

  /* rise time of inverter 1's output */
  double rt;
  /* fall time of inverter 2's output */
  double ft;
  double timeconst;

  timeconst = (drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
               gate_C(min_w_pmos + g_tp.min_w_nmos_, 0)) *
              tr_R_on(min_w_pmos, PCH, 1);
  rt = horowitz(0, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, FALL) /
       (deviceType->Vdd - deviceType->Vth);
  timeconst = (drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
               gate_C(min_w_pmos + g_tp.min_w_nmos_, 0)) *
              tr_R_on(g_tp.min_w_nmos_, NCH, 1);
  ft = horowitz(rt, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, RISE) / deviceType->Vth;
  return ft;
}

double Wire::signal_rise_time() {

  /* rise time of inverter 1's output */
  double ft;
  /* fall time of inverter 2's output */
  double rt;
  double timeconst;

  timeconst = (drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
               gate_C(min_w_pmos + g_tp.min_w_nmos_, 0)) *
              tr_R_on(g_tp.min_w_nmos_, NCH, 1);
  rt        = horowitz(0, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, RISE) / deviceType->Vth;
  timeconst = (drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
               gate_C(min_w_pmos + g_tp.min_w_nmos_, 0)) *
              tr_R_on(min_w_pmos, PCH, 1);
  ft = horowitz(rt, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, FALL) /
       (deviceType->Vdd - deviceType->Vth);
  return ft; // sec
}

/* Wire resistance and capacitance calculations
 *   wire width
 *
 *    /__/
 *   |  |
 *   |  |  height = ASPECT_RATIO*wire width (ASPECT_RATIO = 2.2, ref: ITRS)
 *   |__|/
 *
 *   spacing between wires in same level = wire width
 *   spacing between wires in adjacent levels = wire width
 */

double Wire::wire_cap(double len /* in m */) {
  double sidewall, adj, tot_cap;
  double wire_height;

  wire_height = wire_width / w_scale * g_tp.aspect_ratio; // Sheng: assume height does not changes.

  // capacitance between wires in the same level
  sidewall = g_tp.miller_value * g_tp.horiz_dielectric_constant * (wire_height / wire_spacing) * 8.8542e-12;

  // capacitance between wires in adjacent levels
  adj = g_tp.miller_value * g_tp.vert_dielectric_constant * (w_scale)*8.8542e-12;

  tot_cap = (sidewall + adj + (deviceType->C_fringe * 1e6)); // F/m

  return (tot_cap * len); // (F)
}

double Wire::wire_res(double len /*(in m)*/) {

#define dish 1.1

  PRINTDW(fprintf(stderr,
                  "wire_res: len %f, w_scaling %f,"
                  " s_scaling %f, res %g\n",
                  len, w_scale, s_scale, (dish * resistivity * len / (g_tp.aspect_ratio * (wire_width / w_scale) * wire_width))));

  return (dish * resistivity * 1e-6 * len / (g_tp.aspect_ratio * wire_width / w_scale * wire_width)); // Sheng: bug fix
}

/*
 * Calculates the delay, power and area of the transmitter circuit.
 *
 * The transmitter delay is the sum of nand gate delay, inverter delay
 * low swing nmos delay, and the wire delay
 * (ref: Technical report 6)
 */
void Wire::low_swing_model() {
  double len  = wire_length;
  double beta = pmos_to_nmos_sz_ratio();

  PRINTDW(cout >> "low_swing_model: length of the wire " >> len >> endl);

  double inputrise = (in_rise_time == 0) ? signal_rise_time() : in_rise_time;

  /* Final nmos low swing driver size calculation:
   * Try to size the driver such that the delay
   * is less than 8FO4.
   * If the driver size is greater than
   * the max allowable size, assume max size for the driver.
   * In either case, recalculate the delay using
   * the final driver size assuming slow input with
   * finite rise time instead of ideal step input
   *
   * (ref: Technical report 6)
   */
  double cwire = wire_cap(len); /* load capacitance */
  double rwire = wire_res(len);

#define RES_ADJ (8.6) // Increase in resistance due to low driving vol.

  double driver_res = (-8 * g_tp.FO4 / (log(0.5) * cwire)) / RES_ADJ;
  double nsize      = R_to_w(driver_res, NCH);

  nsize = MIN(nsize, g_tp.max_w_nmos_);
  nsize = MAX(nsize, g_tp.min_w_nmos_);

  if(rwire * cwire > 8 * g_tp.FO4) {
    nsize = g_tp.max_w_nmos_;
  }

  // size the inverter appropriately to minimize the transmitter delay
  // Note - In order to minimize leakage, we are not adding a set of inverters to
  // bring down delay. Instead, we are sizing the single gate
  // based on the logical effort.
  double st_eff   = sqrt((2 + beta / 1 + beta) * gate_C(nsize, 0) / (gate_C(2 * g_tp.min_w_nmos_, 0) + gate_C(2 * min_w_pmos, 0)));
  double req_cin  = ((2 + beta / 1 + beta) * gate_C(nsize, 0)) / st_eff;
  double inv_size = req_cin / (gate_C(min_w_pmos, 0) + gate_C(g_tp.min_w_nmos_, 0));
  inv_size        = MAX(inv_size, 1);

  /* nand gate delay */
  double res_eq = (2 * tr_R_on(g_tp.min_w_nmos_, NCH, 1));
  double cap_eq = 2 * drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
                  drain_C_(2 * g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + gate_C(inv_size * g_tp.min_w_nmos_, 0) +
                  gate_C(inv_size * min_w_pmos, 0);

  double timeconst = res_eq * cap_eq;

  delay             = horowitz(inputrise, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, RISE);
  double temp_power = cap_eq * deviceType->Vdd * deviceType->Vdd;

  inputrise = delay / (deviceType->Vdd - deviceType->Vth); /* for the next stage */

  /* Inverter delay:
   * The load capacitance of this inv depends on
   * the gate capacitance of the final stage nmos
   * transistor which in turn depends on nsize
   */
  res_eq = tr_R_on(inv_size * min_w_pmos, PCH, 1);
  cap_eq = drain_C_(inv_size * min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) +
           drain_C_(inv_size * g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + gate_C(nsize, 0);
  timeconst = res_eq * cap_eq;

  delay += horowitz(inputrise, timeconst, deviceType->Vth / deviceType->Vdd, deviceType->Vth / deviceType->Vdd, FALL);
  temp_power += cap_eq * deviceType->Vdd * deviceType->Vdd;

  PRINTDW(cout >> "transmitter_delay: nand gate + inv delay, "
                  "power " >>
          delay >> ", " >> temp_power * 2 >> endl);

  transmitter.delay                = delay;
  transmitter.power.readOp.dynamic = temp_power * 2; /* since it is a diff. model*/
  transmitter.power.readOp.leakage = deviceType->Vdd * (4 * cmos_Isub_leakage(g_tp.min_w_nmos_, min_w_pmos, 2, nand) +
                                                        4 * cmos_Isub_leakage(g_tp.min_w_nmos_, min_w_pmos, 1, inv));

  transmitter.power.readOp.gate_leakage = deviceType->Vdd * (4 * cmos_Ig_leakage(g_tp.min_w_nmos_, min_w_pmos, 2, nand) +
                                                             4 * cmos_Ig_leakage(g_tp.min_w_nmos_, min_w_pmos, 1, inv));

  inputrise = delay / deviceType->Vth;

  /* nmos delay + wire delay */
  cap_eq = cwire + drain_C_(nsize, NCH, 1, 1, g_tp.cell_h_def) * 2 + nsense * sense_amp_input_cap(); //+receiver cap
  /*
   * NOTE: nmos is used as both pull up and pull down transistor
   * in the transmitter. This is because for low voltage swing, drive
   * resistance of nmos is less than pmos
   * (for a detailed graph ref: On-Chip Wires: Scaling and Efficiency)
   */
  timeconst = (tr_R_on(nsize, NCH, 1) * RES_ADJ) * (cwire + drain_C_(nsize, NCH, 1, 1, g_tp.cell_h_def) * 2) + rwire * cwire / 2 +
              (tr_R_on(nsize, NCH, 1) + rwire) * nsense * sense_amp_input_cap();

  /*
   * since we are pre-equalizing and overdriving the low
   * swing wires, the net time constant is less
   * than the actual value
   */
  delay += horowitz(inputrise, timeconst, deviceType->Vth / deviceType->Vdd, .25, 0);
#define VOL_SWING .1
  temp_power += cap_eq * VOL_SWING * .400; /* .4v is the over drive voltage */
  temp_power *= 2;                         /* differential wire */

  l_wire.delay                = delay - transmitter.delay;
  l_wire.power.readOp.dynamic = temp_power - transmitter.power.readOp.dynamic;
  l_wire.power.readOp.leakage = deviceType->Vdd * (4 * cmos_Isub_leakage(nsize, 0, 1, nmos));

  l_wire.power.readOp.gate_leakage = deviceType->Vdd * (4 * cmos_Ig_leakage(nsize, 0, 1, nmos));

  PRINTDW(cout >> "transmitter_delay: nand gate + inv "
                  "+ nmos + wire delay, power " >>
          delay >> ", " >> temp_power >> endl);

  // double rt = horowitz(inputrise, timeconst, deviceType->Vth/deviceType->Vdd,
  //    deviceType->Vth/deviceType->Vdd, RISE)/deviceType->Vth;

  delay += g_tp.sense_delay;

  sense_amp.delay                     = g_tp.sense_delay;
  out_rise_time                       = g_tp.sense_delay / (deviceType->Vth);
  sense_amp.power.readOp.dynamic      = g_tp.sense_dy_power;
  sense_amp.power.readOp.leakage      = 0; // FIXME
  sense_amp.power.readOp.gate_leakage = 0;
  PRINTDW(cout >> "sense_amp: power " >> sense_amp.power.readOp.dynamic >> "input cap " >> sense_amp_input_cap() >> endl);

  power.readOp.dynamic = temp_power;
  power.readOp.leakage = transmitter.power.readOp.leakage + l_wire.power.readOp.leakage + sense_amp.power.readOp.leakage;
  power.readOp.gate_leakage =
      transmitter.power.readOp.gate_leakage + l_wire.power.readOp.gate_leakage + sense_amp.power.readOp.gate_leakage;
}

double Wire::sense_amp_input_cap() {
  return drain_C_(g_tp.w_iso, PCH, 1, 1, g_tp.cell_h_def) + gate_C(g_tp.w_sense_en + g_tp.w_sense_n, 0) +
         drain_C_(g_tp.w_sense_n, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(g_tp.w_sense_p, PCH, 1, 1, g_tp.cell_h_def);
}

void Wire::delay_optimal_wire() {
  double len = wire_length;
  // double min_wire_width = wire_width; //m
  double beta      = pmos_to_nmos_sz_ratio();
  double switching = 0; // switching energy
  double short_ckt = 0; // short-circuit energy
  double tc        = 0; // time constant
  // input cap of min sized driver
  double input_cap = gate_C(g_tp.min_w_nmos_ + min_w_pmos, 0);

  // output parasitic capacitance of
  // the min. sized driver
  double out_cap = drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) + drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def);
  // drive resistance
  double out_res = (tr_R_on(g_tp.min_w_nmos_, NCH, 1) + tr_R_on(min_w_pmos, PCH, 1)) / 2;
  double wr      = wire_res(len); // ohm

  // wire cap /m
  double wc = wire_cap(len);

  // size the repeater such that the delay of the wire is minimum
  double repeater_scaling = sqrt(out_res * wc / (wr * input_cap)); // len will cancel

  // calc the optimum spacing between the repeaters (m)

  repeater_spacing = sqrt(2 * out_res * (out_cap + input_cap) / ((wr / len) * (wc / len)));
  repeater_size    = repeater_scaling;
  PRINTDW(cout >> "delay_optimal_wire: repeater spacing (m) " >> repeater_spacing >> " repeater sizing " repeater_scaling >> endl);

  switching = (repeater_scaling * (input_cap + out_cap) + repeater_spacing * (wc / len)) * deviceType->Vdd * deviceType->Vdd;

  tc = out_res * (input_cap + out_cap) + out_res * wc / len * repeater_spacing / repeater_scaling +
       wr / len * repeater_spacing * input_cap * repeater_scaling +
       0.5 * (wr / len) * (wc / len) * repeater_spacing * repeater_spacing;

  delay = 0.693 * tc * len / repeater_spacing;

#define Ishort_ckt 65e-6 /* across all tech Ref:Banerjee et al. {IEEE TED} */
  short_ckt = deviceType->Vdd * g_tp.min_w_nmos_ * Ishort_ckt * 1.0986 * repeater_scaling * tc;

  area.set_area((len / repeater_spacing) *
                compute_gate_area(INV, 1, min_w_pmos * repeater_scaling, g_tp.min_w_nmos_ * repeater_scaling, g_tp.cell_h_def));
  power.readOp.dynamic = ((len / repeater_spacing) * (switching + short_ckt));
  power.readOp.leakage =
      ((len / repeater_spacing) * deviceType->Vdd *
       cmos_Isub_leakage(g_tp.min_w_nmos_ * repeater_scaling, beta * g_tp.min_w_nmos_ * repeater_scaling, 1, inv));
  power.readOp.gate_leakage =
      ((len / repeater_spacing) * deviceType->Vdd *
       cmos_Ig_leakage(g_tp.min_w_nmos_ * repeater_scaling, beta * g_tp.min_w_nmos_ * repeater_scaling, 1, inv));
}

// calculate power/delay values for wires with suboptimal repeater sizing/spacing
void Wire::init_wire() {
  wire_length = 1;
  delay_optimal_wire();
  double   sp, si;
  powerDef pow;
  si = repeater_size;
  sp = repeater_spacing;
  sp *= 1e6; // in microns

  double i, j, del;
  repeated_wire.push_back(Component());
  for(j = sp; j < 4 * sp; j += 100) {
    for(i = si; i > 1; i--) {
      pow = wire_model(j * 1e-6, i, &del);
      if(j == sp && i == si) {
        global.delay  = del;
        global.power  = pow;
        global.area.h = si;
        global.area.w = sp * 1e-6; // m
      }
      //      cout << "Repeater size - "<< i <<
      //        " Repeater spacing - " << j <<
      //        " Delay - " << del <<
      //        " PowerD - " << pow.readOp.dynamic <<
      //        " PowerL - " << pow.readOp.leakage <<endl;
      repeated_wire.back().delay        = del;
      repeated_wire.back().power.readOp = pow.readOp;
      repeated_wire.back().area.w       = j * 1e-6; // m
      repeated_wire.back().area.h       = i;
      repeated_wire.push_back(Component());
    }
  }
  repeated_wire.pop_back();
  update_fullswing();
}

void Wire::update_fullswing() {

  list<Component>::iterator citer;
  double                    del[4];
  del[3] = this->global.delay + this->global.delay * .3;
  del[2] = global.delay + global.delay * .2;
  del[1] = global.delay + global.delay * .1;
  del[0] = global.delay + global.delay * .05;
  double threshold;
  double ncost;
  double cost;
  int    i = 4;
  while(i > 0) {
    threshold = del[i - 1];
    cost      = BIGNUM;
    for(citer = repeated_wire.begin(); citer != repeated_wire.end(); citer++) {
      if(citer->delay > threshold) {
        citer = repeated_wire.erase(citer);
        citer--;
      } else {
        ncost =
            citer->power.readOp.dynamic / global.power.readOp.dynamic + citer->power.readOp.leakage / global.power.readOp.leakage;
        if(ncost < cost) {
          cost = ncost;
          if(i == 4) {
            global_30.delay = citer->delay;
            global_30.power = citer->power;
            global_30.area  = citer->area;
          } else if(i == 3) {
            global_20.delay = citer->delay;
            global_20.power = citer->power;
            global_20.area  = citer->area;
          } else if(i == 2) {
            global_10.delay = citer->delay;
            global_10.power = citer->power;
            global_10.area  = citer->area;
          } else if(i == 1) {
            global_5.delay = citer->delay;
            global_5.power = citer->power;
            global_5.area  = citer->area;
          }
        }
      }
    }
    i--;
  }
}

powerDef Wire::wire_model(double space, double size, double *delay) {
  powerDef ptemp;
  double   len = 1;
  // double min_wire_width = wire_width; //m
  double beta = pmos_to_nmos_sz_ratio();
  // switching energy
  double switching = 0;
  // short-circuit energy
  double short_ckt = 0;
  // time constant
  double tc = 0;
  // input cap of min sized driver
  double input_cap = gate_C(g_tp.min_w_nmos_ + min_w_pmos, 0);

  // output parasitic capacitance of
  // the min. sized driver
  double out_cap = drain_C_(min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) + drain_C_(g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def);
  // drive resistance
  double out_res = (tr_R_on(g_tp.min_w_nmos_, NCH, 1) + tr_R_on(min_w_pmos, PCH, 1)) / 2;
  double wr      = wire_res(len); // ohm

  // wire cap /m
  double wc = wire_cap(len);

  repeater_spacing = space;
  repeater_size    = size;

  switching = (repeater_size * (input_cap + out_cap) + repeater_spacing * (wc / len)) * deviceType->Vdd * deviceType->Vdd;

  tc = out_res * (input_cap + out_cap) + out_res * wc / len * repeater_spacing / repeater_size +
       wr / len * repeater_spacing * out_cap * repeater_size + 0.5 * (wr / len) * (wc / len) * repeater_spacing * repeater_spacing;

  *delay = 0.693 * tc * len / repeater_spacing;

#define Ishort_ckt 65e-6 /* across all tech Ref:Banerjee et al. {IEEE TED} */
  short_ckt = deviceType->Vdd * g_tp.min_w_nmos_ * Ishort_ckt * 1.0986 * repeater_size * tc;

  ptemp.readOp.dynamic = ((len / repeater_spacing) * (switching + short_ckt));
  ptemp.readOp.leakage = ((len / repeater_spacing) * deviceType->Vdd *
                          cmos_Isub_leakage(g_tp.min_w_nmos_ * repeater_size, beta * g_tp.min_w_nmos_ * repeater_size, 1, inv));

  ptemp.readOp.gate_leakage = ((len / repeater_spacing) * deviceType->Vdd *
                               cmos_Ig_leakage(g_tp.min_w_nmos_ * repeater_size, beta * g_tp.min_w_nmos_ * repeater_size, 1, inv));

  return ptemp;
}

void Wire::print_wire() {

  cout << "\nWire Properties:\n\n";
  cout << "  Delay Optimal\n\tRepeater size - " << global.area.h << " \n\tRepeater spacing - " << global.area.w * 1e3
       << " (mm)"
          " \n\tDelay - "
       << global.delay * 1e6
       << " (ns/mm)"
          " \n\tPowerD - "
       << global.power.readOp.dynamic * 1e6
       << " (nJ/mm)"
          " \n\tPowerL - "
       << global.power.readOp.leakage
       << " (mW/mm)"
          " \n\tPowerLgate - "
       << global.power.readOp.gate_leakage << " (mW/mm)\n";
  cout << endl;

  cout << "  5% Overhead\n\tRepeater size - " << global_5.area.h << " \n\tRepeater spacing - " << global_5.area.w * 1e3
       << " (mm)"
          " \n\tDelay - "
       << global_5.delay * 1e6
       << " (ns/mm)"
          " \n\tPowerD - "
       << global_5.power.readOp.dynamic * 1e6
       << " (nJ/mm)"
          " \n\tPowerL - "
       << global_5.power.readOp.leakage
       << " (mW/mm)"
          " \n\tPowerLgate - "
       << global_5.power.readOp.gate_leakage << " (mW/mm)\n";
  cout << endl;
  cout << "  10% Overhead\n\tRepeater size - " << global_10.area.h << " \n\tRepeater spacing - " << global_10.area.w * 1e3
       << " (mm)"
          " \n\tDelay - "
       << global_10.delay * 1e6
       << " (ns/mm)"
          " \n\tPowerD - "
       << global_10.power.readOp.dynamic * 1e6
       << " (nJ/mm)"
          " \n\tPowerL - "
       << global_10.power.readOp.leakage
       << " (mW/mm)"
          " \n\tPowerLgate - "
       << global_10.power.readOp.gate_leakage << " (mW/mm)\n";
  cout << endl;
  cout << "  20% Overhead\n\tRepeater size - " << global_20.area.h << " \n\tRepeater spacing - " << global_20.area.w * 1e3
       << " (mm)"
          " \n\tDelay - "
       << global_20.delay * 1e6
       << " (ns/mm)"
          " \n\tPowerD - "
       << global_20.power.readOp.dynamic * 1e6
       << " (nJ/mm)"
          " \n\tPowerL - "
       << global_20.power.readOp.leakage
       << " (mW/mm)"
          " \n\tPowerLgate - "
       << global_20.power.readOp.gate_leakage << " (mW/mm)\n";
  cout << endl;
  cout << "  30% Overhead\n\tRepeater size - " << global_30.area.h << " \n\tRepeater spacing - " << global_30.area.w * 1e3
       << " (mm)"
          " \n\tDelay - "
       << global_30.delay * 1e6
       << " (ns/mm)"
          " \n\tPowerD - "
       << global_30.power.readOp.dynamic * 1e6
       << " (nJ/mm)"
          " \n\tPowerL - "
       << global_30.power.readOp.leakage
       << " (mW/mm)"
          " \n\tPowerLgate - "
       << global_30.power.readOp.gate_leakage << " (mW/mm)\n";
  cout << endl;
  cout << endl;
}
