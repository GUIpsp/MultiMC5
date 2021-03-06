/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INSTANCETYPE_H
#define INSTANCETYPE_H

#include <QObject>

#include "instanceloader.h"

class InstVersionList;

//! The InstanceTypeInterface's interface ID.
#define InstanceTypeInterface_IID "net.forkk.MultiMC.InstanceTypeInterface/0.1"

/*!
 * \brief The InstanceType class is an interface for all instance types.
 * InstanceTypes are usually provided by plugins.
 * It handles loading and creating instances of a certain type. There should be 
 * one of these for each type of instance and they should be registered with the 
 * InstanceLoader.
 * To create an instance, the InstanceLoader calls the type's createInstance() 
 * function. Loading is done through the loadInstance() function.
 */
class InstanceTypeInterface
{
public:
	friend class InstanceLoader;
	
	/*!
	 * \brief Gets the ID for this instance type. 
	 *        The type ID should be unique as it is used to identify the type 
	 *        of instances when they are loaded.
	 *        Changing this value at runtime results in undefined behavior.
	 * \return This instance type's ID string. 
	 */
	virtual QString typeID() const = 0;
	
	/*!
	 * \brief Gets the name of this instance type as it is displayed to the user.
	 * \return The instance type's display name.
	 */
	virtual QString displayName() const = 0;
	
	/*!
	 * \brief Gets a longer, more detailed description of this instance type.
	 * \return The instance type's description.
	 */
	virtual QString description() const = 0;
	
	/*!
	 * \brief Gets the version list for this instance type.
	 * \return A pointer to this instance type's version list.
	 */
	virtual InstVersionList *versionList() const = 0;
	
protected:
	/*!
	 * \brief Creates an instance and stores it in inst.
	 * \param inst Pointer to store the created instance in.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 *         TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 *         InstExists if the given instance directory is already an instance.
	 */
	virtual InstanceLoader::InstTypeError createInstance(Instance *&inst, const QString &instDir) const = 0;
	
	/*!
	 * \brief Loads an instance from the given directory.
	 * \param inst Pointer to store the loaded instance in.
	 * \param instDir The instance's directory.
	 * \return An InstTypeError error code.
	 *         TypeNotRegistered if the given type is not registered with the InstanceLoader.
	 *         NotAnInstance if the given instance directory isn't a valid instance.
	 *         WrongInstType if the given instance directory's type isn't an instance of this type.
	 */
	virtual InstanceLoader::InstTypeError loadInstance(Instance *&inst, const QString &instDir) const = 0;
};

Q_DECLARE_INTERFACE(InstanceTypeInterface, InstanceTypeInterface_IID)

#endif // INSTANCETYPE_H
