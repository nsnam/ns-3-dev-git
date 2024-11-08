/*
 * Copyright (c) 2013 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef STATS_H
#define STATS_H

// Header file used only to define the stats group in Doxygen

/**
 * @defgroup stats Statistics
 *
 * The statistics module includes some useful features to ease data collection from experiments.
 * In particular the following features are included:
 * <ul>
 * <li> The core framework and two basic data collectors: A counter, and a min/max/avg/total
 * observer.</li> <li> Extensions of those to easily work with times and packets.</li> <li>
 * Plaintext output formatted for OMNet++.</li> <li> Database output using SQLite, a standalone,
 * lightweight, high performance SQL engine.</li> <li> Mandatory and open ended metadata for
 * describing and working with runs.</li>
 * </ul>
 *
 * See the manual for a complete documentation.
 */

/**
 * @ingroup stats
 * @defgroup aggregator Data Aggregators
 *
 * Data aggregators are classes used to collect data and produce output
 * specialized for various purpose, e.g., Gnuplot, file output, etc.
 */

/**
 * @ingroup stats
 * @defgroup probes Probes
 *
 * Probes are used to probe an underlying ns3 TraceSource exporting
 * its value.  This probe usually exports a trace source "Output".
 * The Output trace source emits a value when either the trace source
 * emits a new value, or when SetValue () is called.
 *
 * Probes are a special kind of Trace Source.
 */

/**
 * @ingroup stats
 * @defgroup gnuplot Gnuplot
 *
 * Classes in Gnuplot group are used to collect and prepare and output data
 * for subsequent processing by Gnuplot.
 */

/**
 * @ingroup stats
 * @defgroup dataoutput Data Output
 *
 * Classes in Data Output group are used to collect and prepare and output data
 * for subsequent output in a specific format, e.g., Omnet++, SQLite, etc.
 */

/**
 * @ingroup stats
 * @ingroup tests
 * @defgroup stats-tests Statistics module tests
 */

#endif /* STATS_H */
