/*
 * Autopin+ - Automatic thread-to-core-pinning tool
 * Copyright (C) 2012 LRR
 *
 * Author:
 * Florian Walter
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact address:
 * LRR (I10)
 * Technische Universitaet Muenchen
 * Boltzmannstr. 3
 * D-85784 Garching b. Muenchen
 * http://autopin.in.tum.de
 */

#pragma once

#include <AutopinPlus/AutopinContext.h>
#include <AutopinPlus/Configuration.h>
#include <AutopinPlus/Error.h>
#include <AutopinPlus/ObservedProcess.h>
#include <AutopinPlus/OS/OSServices.h>
#include <AutopinPlus/PerformanceMonitor.h>
#include <deque>
#include <map>
#include <QObject>
#include <memory>

namespace AutopinPlus {

/*!
 * \brief Base class for control strategies
 *
 * A control strategy determines how autopin+ assigns tasks to cpu cores.
 */
class ControlStrategy : public QObject {
	Q_OBJECT

  public:
	/*!
	 * \brief Constructor
	 *
	 * \param[in] config	Reference to the current Configuration instance
	 * \param[in] proc		Reference to the observed process
	 * \param[in] service	Reference to the current OSServices instance
	 * \param[in] monitors	Reference to a list of available instances of PerformanceMonitor
	 * \param[in] context	Refernnce to the context of the object calling the constructor
	 */
	ControlStrategy(const Configuration &config, const ObservedProcess &proc, OS::OSServices &service,
					const PerformanceMonitor::monitor_list &monitors, AutopinContext &context);

	/*!
	 * \brief Initializes the control strategy
	 */
	virtual void init();

	/*!
	 * \brief Get the name of the control strategy
	 *
	 * \return The name of the control strategy
	 */
	QString getName();

	/*!
	 * \brief Returns the configuration options of the control strategy
	 *
	 * \return A list with the configuration options
	 */
	virtual Configuration::configopts getConfigOpts() = 0;

	/*!
	 * \brief Data structure that maps tasks to cores
	 */
	using autopin_pinning = std::deque<int>;

	/*!
	 * \brief Data structure for storing a list of pinnings
	 */
	using pinning_list = std::deque<autopin_pinning>;

	/*!
	 * \brief Data type for storing a pinning with its result
	 */
	using pinning_result = std::pair<autopin_pinning, double>;

  public slots:
	/*!
	 * \brief Starts the strategy when autopin+ has finished initialization
	 */
	virtual void slot_autopinReady() = 0;

	/*!
	 * \brief Handles the creation of a new tasks
	 *
	 * \param[in] tid The tid of the task
	 */
	virtual void slot_TaskCreated(int tid);

	/*!
	 * \brief Handles the termination of a task
	 *
	 * \param[in] tid The tid of the task
	 */
	virtual void slot_TaskTerminated(int tid);

	/*!
	 * \brief Handles transitions between execution phases
	 *
	 * \param[in] newphase 	The new phase
	 */
	virtual void slot_PhaseChanged(int newphase);

	/*!
	 * \brief Handles user-defined messages from the communication channel
	 *
	 * \param[in] arg The argument value of the messages
	 * \param[in] val The value stored in the messages
	 */
	virtual void slot_UserMessage(int arg, double val);

  protected:
	/*!
	 * \brief Data structure for storing a list of pinnings
	 *
	 * In contrast to ProcessTree::autopin_tid_list this data structure
	 * supports the sorting of tasks.
	 */
	using autopin_tasklist = std::deque<int>;

	/*!
	 * \brief Data structure which maps tids to the corresponding sortids
	 *
	 * sortids are used for sorting tids in a consistent way
	 */
	using sortmap = std::map<int, int>;

	/*!
	 * \brief Reads pinnings from the configuration
	 *
	 * \param[in] opt Name of the configuration option where
	 * 	the pinnings are stored.
	 * \return A list of pinnings
	 */
	pinning_list readPinnings(QString opt);

	/*!
	 * \brief Factory function for the pinning history
	 *
	 * Reads the configuration and creates the requested
	 * pinning history. The pinning history which is created
	 * depends on the suffix of the history file:
	 */
	void createPinningHistory();

	//@{
	/*!
	 * Variables for storing runtime information in the constructor
	 */
	const Configuration &config;
	const ObservedProcess &proc;
	OS::OSServices &service;
	const PerformanceMonitor::monitor_list &monitors;
	//@}

	/*!
	 * The runtime context
	 */
	AutopinContext &context;

	/*!
	 * Current tasks of the observed process
	 */
	autopin_tasklist tasks;

	/*!
	 * \brief Get running tasks
	 *
	 * Refreshes the current tasks of the observed process and stores
	 * them in tasks sorted by tid.
	 */
	void refreshTasks();

	/*!
	 * Name of the control strategy
	 */
	QString name;

	/*!
	 * \brief Data structure for sorting tasks
	 *
	 * This struct is used in refreshTasks() and serves as comparison function for the
	 * call of the sort function. Moreover, it stores a sort id for every. This id has
	 * to be assigned before calling sort. The sorting is done with respect to the sort
	 * ids (e. g. they could contain the creation time of a task) in ascending order. If
	 * two sort ids are equal the tids are considered.
	 */
	struct tasksort {
		sortmap sort_tasks;
		bool operator()(int tida, int tidb) {
			if (sort_tasks[tida] < sort_tasks[tidb])
				return true;
			else if (sort_tasks[tida] > sort_tasks[tidb])
				return false;
			else
				return (tida < tidb);
		}
	};
};

} // namespace AutopinPlus
