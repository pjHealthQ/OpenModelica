// $Id$
/**
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR 
 * THIS OSMC PUBLIC LICENSE (OSMC-PL). 
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
 * OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE. 
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or  
 * http://www.openmodelica.org, and in the OpenModelica distribution. 
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 * Main contributor 2010, Hubert Thierot, CEP - ARMINES (France)
 * Main contributor 2010, Hubert Thierot, CEP - ARMINES (France)

 	@file ModClassTree.cpp
 	@brief Comments for file documentation.
 	@author Hubert Thieriot, hubert.thieriot@mines-paristech.fr
 	Company : CEP - ARMINES (France)
 	http://www-cep.ensmp.fr/english/
 	@version 0.9 

  */
#include "ModClassTree.h"

#include <QtGui>


        ModClassTree::ModClassTree(ModReader* modReader,MOomc* moomc,QObject *parent)
: QAbstractItemModel(parent)
{
    _modReader = modReader;
    _moomc = moomc;
    _rootElement = new ModClass(_moomc);

    _enabled = true;

    _showOnlyComponents = false;

    // each change in _rootElement descendants implicates all tree redraw
	// to optimize (more signals and index find function from ModClass*)
//    connect(_rootElement,SIGNAL(modified()),this,SLOT(allDataChanged()));
//    connect(_rootElement,SIGNAL(cleared()),this,SLOT(allDataCleared()));
    connect(_rootElement,SIGNAL(deleted()),this,SLOT(onRootDeleted()));
}

ModClassTree::~ModClassTree()
{
    clear();
    delete _rootElement;
}


void ModClassTree::clear()
{
    beginResetModel();
    _rootElement->clear();
    endResetModel();
}

bool ModClassTree::addModClass(ModClass* parent,QString className,QString filePath)
{

    bool ok=false;
    ModClass* newClass = _modReader->newModClass(className,filePath);

    if(newClass)
        ok = addChild(parent,newClass);
    return ok;
}

bool ModClassTree::addChild(ModClass* parent,ModClass* child)
{
    bool ok;
    QModelIndex parentIndex = indexOf(parent);
    beginInsertRows(parentIndex,parent->childCount(),parent->childCount());
    ok = parent->addChild(child);
    endInsertRows();
    return ok;
}



void ModClassTree::readFromOmc(ModClass* parent,int depthMax,int curDepth)
{
        if(parent->_readMutex.tryLock())
        {
                if((curDepth<depthMax)&&!parent->childrenReaden())
                {
                        ModClass* newElement;

                        bool readPackages = (parent->getClassRestr()==Modelica::PACKAGE);
                        bool readModels = (parent->getClassRestr()==Modelica::PACKAGE);
                        bool readComponents = (parent->getClassRestr()==Modelica::MODEL);

                        QString fullParentName = parent->name(Modelica::FULL);

                        // read packages
                        if(readPackages)
                        {
                                QStringList packageNames = _moomc->getPackages(fullParentName);
                                for(int i=0;i<packageNames.size();i++)
                                {
                                        newElement = new ModPackage(_moomc,parent,packageNames.at(i));
                                        if(addChild(parent,newElement))
                                                readFromOmc(newElement,depthMax,curDepth+1);
                                }
                        }

                        // read models
                        if(readModels)
                        {
                                QStringList modelNames = _moomc->getModels(fullParentName);
                                for(int i=0;i<modelNames.size();i++)
                                {
                                        newElement = new ModModel(_moomc,parent,modelNames.at(i));
                                        if(addChild(parent,newElement))
                                                readFromOmc(newElement,depthMax,curDepth+1);
                                }
                        }

                        //read components
                        if(readComponents)
                        {
                                QStringList compNames;
                                QStringList compClasses;
                                _moomc->getContainedComponents(fullParentName,compNames,compClasses);
                                for(int i=0;i<compNames.size();i++)
                                {
                                        newElement = new ModComponent(_moomc,parent,compNames.at(i),compClasses.at(i));
                                        if(addChild(parent,newElement))
                                                readFromOmc(newElement,depthMax,curDepth+1);
                                }
                        }
                        parent->setChildrenReaden(true);

                        parent->emitModified();


                }
                parent->_readMutex.unlock();
        }
}


void ModClassTree::readFromOmcV2(ModClass* parent,int depthMax,QString direction,int curDepth)
{
        if(parent->_readMutex.tryLock())
        {
                if((curDepth<=depthMax)&&!parent->childrenReaden())
                {
                        QString childrenDirection = direction.section(".",curDepth+1,curDepth+1);
                        QString fullParentClass = parent->getModClassName();

                        ModClass* newElement;

                        //read packages, models and components
                        QStringList packagesClasses,modelsClasses,compsNames,compsClasses;
                        _moomc->readElementInfos(fullParentClass,packagesClasses,modelsClasses,compsNames,compsClasses);

                        // get inherited components
                        QStringList inheritedCompNames,inheritedCompClasses;
                        _moomc->getInheritedComponents(fullParentClass,inheritedCompNames,inheritedCompClasses);
                        compsNames << inheritedCompNames;
                        compsClasses << inheritedCompClasses;

                        // adding child packages and read them
                        for(int i=0;i<packagesClasses.size();i++)
                        {
                                newElement = new ModPackage(_moomc,parent,packagesClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==packagesClasses.at(i)))
                                                readFromOmcV2(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child models and read them
                        for(int i=0;i<modelsClasses.size();i++)
                        {
                                newElement = new ModModel(_moomc,parent,modelsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==modelsClasses.at(i)))
                                                readFromOmcV2(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child components and read them
                        for(int i=0;i<compsNames.size();i++)
                        {
                                newElement = new ModComponent(_moomc,parent,compsNames.at(i),compsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==compsClasses.at(i)))
                                                readFromOmcV2(newElement,depthMax,direction,curDepth+1);
                        }

                        parent->setChildrenReaden(true);
                        parent->emitModified();


                }
                parent->_readMutex.unlock();
        }
}

void ModClassTree::readFromOmcV3(ModClass* parent,int depthMax,QString direction,int curDepth)
{
        if(parent->_readMutex.tryLock())
        {
                if((curDepth<=depthMax)&&!parent->childrenReaden())
                {
                        QString childrenDirection = direction.section(".",curDepth+1,curDepth+1);
                        QString fullParentClass = parent->getModClassName();
                        QString fullParentName = parent->name(Modelica::FULL);

                        ModClass* newElement;

                        //read packages, models and components
                        QStringList packagesClasses,modelsClasses,compsNames,compsClasses;
                        bool foundInLib = false;


                        ModClass* _corrLibComp = findInDescendants(fullParentName);
                        if(_corrLibComp)
                        {
                                // If class is already loaded in package, use its information
                                foundInLib = true;
                                childrenInfos(_corrLibComp,packagesClasses,modelsClasses,compsNames,compsClasses);
                        }
                        else
                        {
                                // Call parent->_moomc
                                _moomc->readElementInfos(fullParentClass,packagesClasses,modelsClasses,compsNames,compsClasses);
                                // get inherited components
                                QStringList inheritedCompNames,inheritedCompClasses;
                                _moomc->getInheritedComponents(fullParentClass,inheritedCompNames,inheritedCompClasses);
                                compsNames << inheritedCompNames;
                                compsClasses << inheritedCompClasses;
                        }


                        // adding child packages and read them
                        for(int i=0;i<packagesClasses.size();i++)
                        {
                                newElement = new ModPackage(_moomc,parent,packagesClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==packagesClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child models and read them
                        for(int i=0;i<modelsClasses.size();i++)
                        {
                                newElement = new ModModel(_moomc,parent,modelsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==modelsClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child components and read them
                        for(int i=0;i<compsNames.size();i++)
                        {
                                newElement = new ModComponent(_moomc,parent,compsNames.at(i),compsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==compsClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        parent->setChildrenReaden(true);

                        parent->emitModified();


                }
                parent->_readMutex.unlock();
        }
}




void ModClassTree::readFromOmcV4(ModClass* parent,int depthMax,QString direction,int curDepth)
{
        if(parent->_readMutex.tryLock())
        {
                if((curDepth<=depthMax)&&!parent->childrenReaden())
                {
                        QString childrenDirection = direction.section(".",curDepth+1,curDepth+1);
                        QString fullParentClass = parent->getModClassName();
                        QString fullParentName = parent->name(Modelica::FULL);

                        ModClass* newElement;

                        //read packages, models and components
                        QStringList packagesClasses,modelsClasses,compsNames,compsClasses;
                        bool foundInLib = false;


                        ModClass* _corrLibComp = findInDescendants(fullParentName);
                        if(_corrLibComp)
                        {
                                // If class is already loaded in package, use its information
                                foundInLib = true;
                                childrenInfos(_corrLibComp,packagesClasses,modelsClasses,compsNames,compsClasses);
                        }
                        else
                        {
                                // Call parent->_moomc
                                _moomc->readElementInfos(fullParentClass,packagesClasses,modelsClasses,compsNames,compsClasses);
                                // get inherited components
                                QStringList inheritedCompNames,inheritedCompClasses;
                                _moomc->getInheritedComponents(fullParentClass,inheritedCompNames,inheritedCompClasses);
                                compsNames << inheritedCompNames;
                                compsClasses << inheritedCompClasses;
                        }


                        // adding child packages and read them
                        for(int i=0;i<packagesClasses.size();i++)
                        {
                                newElement = new ModPackage(_moomc,parent,packagesClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==packagesClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child models and read them
                        for(int i=0;i<modelsClasses.size();i++)
                        {
                                newElement = new ModModel(_moomc,parent,modelsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==modelsClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        // adding child components and read them
                        for(int i=0;i<compsNames.size();i++)
                        {
                                newElement = new ModComponent(_moomc,parent,compsNames.at(i),compsClasses.at(i));
                                if(addChild(parent,newElement))
                                        if((childrenDirection=="") || (childrenDirection==compsClasses.at(i)))
                                                readFromOmcV3(newElement,depthMax,direction,curDepth+1);
                        }

                        parent->setChildrenReaden(true);

                        parent->emitModified();


                }
                parent->_readMutex.unlock();
        }
}


ModClass* ModClassTree::findInDescendants(QString fullName,ModClass* parent)
{
    if(parent==NULL)
        parent = _rootElement;


        ModClass* curChild;
        QString curFullName = parent->name(Modelica::FULL);

        int curDepth = parent->depth();
        int lookingDepth = fullName.split(".").size()-1;

        // check if it is this component
        if(curFullName == fullName)
                return parent;

        //first check name compatibility
        if(fullName.indexOf(curFullName)!=0)
                return NULL;

        // then check children are readen
        if(!parent->childrenReaden())
        {
                // if not, check in its direction
                readFromOmcV2(parent,lookingDepth,fullName,curDepth);
        }

        //get child name to visit
        QString childShortName = fullName.section(".",curDepth,curDepth);

        // looking in children
        for(int iChild=0;iChild<parent->childCount();iChild++)
        {
                curChild = parent->child(iChild);
                if(curChild->name(Modelica::SHORT)==childShortName)
                        return findInDescendants(fullName,curChild);
        }
        return NULL;
}



QList<ModClass*> ModClassTree::findCompOfClassInDescendants(QString _className,ModClass* parent)
{
    if(parent==NULL)
        parent = _rootElement;

        ModClass* curComponent;
        QList<ModClass*> curComponents;
        int iChild;

        int nbCompChild = parent->compChildCount();
        int nbModelChild = parent->modelChildCount();
        int nbPackageChild = parent->packageChildCount();

        // looking if one child corresponds
        for(iChild=0;iChild<nbCompChild;iChild++)
        {
                curComponent = parent->compChild(iChild);
                if(curComponent->getFieldValue(ModComponent::MODCLASSNAME)==_className)
                        curComponents.push_back(curComponent);
        }

        //look deeper in components children
        iChild=0;
        while(iChild<nbCompChild)
        {
                curComponents <<  findCompOfClassInDescendants(_className,parent->compChild(iChild));
                iChild++;
        }

        //look deeper in models children
        iChild=0;
        while(iChild<nbModelChild)
        {
                curComponents <<   findCompOfClassInDescendants(_className,parent->modelChild(iChild));
                iChild++;
        }

        //look deeper in packages children
        iChild=0;
        while(iChild<nbPackageChild)
        {
                curComponents <<  findCompOfClassInDescendants(_className,parent->packageChild(iChild));
                iChild++;
        }

        return curComponents;
}


bool ModClassTree::isInDescendants(QString fullName,ModClass* parent)
{
        ModClass* foundClass = findInDescendants(fullName,parent);
        return (bool)(foundClass);
}













int ModClassTree::columnCount(const QModelIndex &parent) const
{
		//return ModClass::nbFields;
	return 1;
}

QVariant ModClassTree::data(const QModelIndex &index, int role) const
{
    if(_enabled)
    {
		if (!index.isValid())
			return QVariant();

		if (role != Qt::DisplayRole && role !=Qt::ToolTipRole)
			return QVariant();

		if(index.column()<0 || index.column()>=ModClass::nbFields)
			return QVariant();

		ModClass *item = static_cast<ModClass*>(index.internalPointer());

		if(item)
		{
            // fullfiling is now done in ::fetchmore
            // if(!item->childrenReaden())
            // readFromOmcV2(item,1);

			if (role == Qt::ToolTipRole)
			{
				QString tooltip = item->getStrToolTip();
				return tooltip;
			}

			return item->getFieldValue(index.column(),role);
		}
		else
		{
			return QVariant();
		}
}
    else
        return QVariant();
}


Qt::ItemFlags ModClassTree::flags(const QModelIndex &index) const
{
    if(!_enabled)
        return Qt::NoItemFlags;

		if (!index.isValid())
			return Qt::ItemIsEnabled;

		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

}

QVariant ModClassTree::headerData(int section, Qt::Orientation orientation,
									int role) const
{
    if(!_enabled)
        return QVariant();

		if(section<0 || section>=ModClass::nbFields)
			return QVariant();

		if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
			return ModClass::sFieldName(section,role);

	return QVariant();
}

 QStringList ModClassTree::mimeTypes() const
 {
     QStringList types;
     types << "application/vnd.text.list";
     return types;
 }

 QMimeData* ModClassTree::mimeData(const QModelIndexList &indexes) const
 {
     QMimeData *mimeData = new QMimeData();
     QByteArray encodedData;

     QDataStream stream(&encodedData, QIODevice::WriteOnly);
	 if(indexes.size()==1)
	 {
		 ModClass* _modEl = (ModClass*)indexes.at(0).internalPointer();
		 if(_modEl)
		 {
			 stream<< _modEl->name(Modelica::FULL);
		 }
	 }

     mimeData->setData("application/vnd.text.list", encodedData);
     return mimeData;
 }

QModelIndex ModClassTree::index(int row, int column, const QModelIndex &parent)
const
{
    if(!_enabled)
        return QModelIndex();

		if(!hasIndex(row,column,parent))
			return QModelIndex();

		ModClass *parentComponent;

		if (!parent.isValid())
            parentComponent = _rootElement;
		else
			parentComponent = static_cast<ModClass*>(parent.internalPointer());

		// looking in children
		int nbPacks = parentComponent->packageChildCount();
		int nbModels = parentComponent->modelChildCount();
		int nbComps = parentComponent->compChildCount();

		if(row<0 || row>= nbPacks+nbModels+nbComps)
			return QModelIndex();


		ModClass *childElement = NULL;
		// start by packages
		if(row<nbPacks)
			childElement = parentComponent->packageChild(row);
		// then models
		else if (row<nbPacks+nbModels)
			childElement = parentComponent->modelChild(row - nbPacks);
		// then components
		else if (row < nbPacks+nbModels+nbComps)
			childElement = parentComponent->compChild(row - nbPacks - nbModels);

		if (childElement)
			return createIndex(row, column, childElement);
		else
			return QModelIndex();
}

QModelIndex ModClassTree::indexOf(ModClass* item,int column)
{
    if(item==_rootElement)
        return createIndex(0,column,item);
    else
        return createIndex(item->indexInParent(),column,item);
}

QModelIndex ModClassTree::parent(const QModelIndex &index) const
{
    if(!_enabled)
         return QModelIndex();

		if (!index.isValid())
			return QModelIndex();

		ModClass *childElement = static_cast<ModClass*>(index.internalPointer());
		
		ModClass *parentElement  = NULL;
        parentElement = childElement->parent();

        if (parentElement == _rootElement)
			return QModelIndex();

		ModClass *grandParentElement = NULL;
        grandParentElement = parentElement->parent();

		//looking for row number of child in parent
		int nbPacks = grandParentElement->packageChildCount();
		int nbModels = grandParentElement->modelChildCount();
		int nbComps = grandParentElement->compChildCount();

		int iC=0;
		bool found = false;
		//start by packages
		while(!found && iC<nbPacks)
		{
			found = (grandParentElement->packageChild(iC)==parentElement);	
			if(!found)
				iC++;
		}

		//then Models
		while(!found && iC<nbPacks+nbModels)
		{
			found = (grandParentElement->modelChild(iC-nbPacks)==parentElement);	
			if(!found)
				iC++;
		}

		// then components
		while(!found && iC<nbPacks + nbModels + nbComps)
		{
			found = (grandParentElement->compChild(iC - nbPacks - nbModels)==parentElement);	
			if(!found)
				iC++;
		}


		if(!found)
		{
			// ERROR
			return QModelIndex();
		}

		return createIndex(iC, 0, parentElement);
}
bool ModClassTree::hasChildren ( const QModelIndex & parent ) const
{
    if(!_enabled)
        return false;

    ModClass *parentElement;

    if (parent.column() > 0)
        return false;

    if (!parent.isValid())
        return (rootElement()->childCount()>0);
    else
        parentElement = static_cast<ModClass*>(parent.internalPointer());

    if(parentElement->childrenReaden())
        return (parentElement->childCount()>0);
    else
    {
        QStringList children = _moomc->getClassNames(parentElement->name());
        return !children.isEmpty();
    }
}
int ModClassTree::rowCount(const QModelIndex &parent) const
{
    if(!_enabled)
        return 0;

		ModClass *parentElement;

		if (parent.column() > 0)
         return 0;

		if (!parent.isValid())
            parentElement = _rootElement;
		else
			parentElement = static_cast<ModClass*>(parent.internalPointer());

		return parentElement->childCount();

}

/**
* \brief
* Return first ModModel in item parents (including himself).
* This function can for instance be used to know which model compile
* in order to update item value.
* @param itemName : full name of item whom who are looking parent model
*/
ModModel* ModClassTree::modelOf(QString itemName)
{
    ModClass* item = findInDescendants(itemName);
    return modelOf(item);
}
/**
* \brief
* Return first ModModel in item parents (including himself).
* This function can for instance be used to know which model compile
* in order to update item value.
*/
ModModel* ModClassTree::modelOf(ModClass* item)
{
    if(item==NULL)
        return NULL;

    if(item->getClassRestr()==Modelica::MODEL)
        return dynamic_cast<ModModel*>(item);
    else
        return dynamic_cast<ModModel*>(modelOf(item->parent()));
}


ModClass* ModClassTree::findItem(QString _fullName)
{
    return findInDescendants(_fullName,_rootElement);
}



void ModClassTree::childrenInfos(ModClass* parent,QStringList &packagesClasses,QStringList &modelsClasses,QStringList &compsNames,QStringList &compsClasses)
{
        // read children if necessary
        if(!parent->childrenReaden())
        {
                int curDepth = parent->depth();
                readFromOmcV2(parent,curDepth+1,QString(),curDepth);
        }

        int nbCompChild = parent->compChildCount();
        int nbModelChild = parent->modelChildCount();
        int nbPackageChild = parent->packageChildCount();

        //clear
        packagesClasses.clear();
        modelsClasses.clear();
        compsClasses.clear();
        compsNames.clear();

        // packages
        for(int i=0;i<nbPackageChild;i++)
                packagesClasses.push_back(parent->packageChild(i)->name(Modelica::SHORT));

        // models
        for(int i=0;i<nbModelChild;i++)
                modelsClasses.push_back(parent->modelChild(i)->name(Modelica::SHORT));

        // components
        for(int i=0;i<nbCompChild;i++)
        {
                compsClasses.push_back(parent->compChild(i)->getFieldValue(ModComponent::MODCLASSNAME).toString());
                compsNames.push_back(parent->compChild(i)->name(Modelica::SHORT));
        }
}


// get all ports of component parent (not only those connected)
QList<ModClass*> ModClassTree::getPorts(ModClass* parent)
{
        QList<ModClass*> ports;

        int curDepth = parent->depth();
        if(!parent->childrenReaden())
        {
                readFromOmcV2(parent,curDepth+1,QString(),curDepth);
        }

        int nbCompChild = parent->compChildCount();
        for(int i=0; i<nbCompChild;i++)
        {
                ModComponent* curComp = (ModComponent*)parent->compChild(i);
                if(_moomc->isConnector(curComp->getFieldValue(ModComponent::MODCLASSNAME).toString()))
                        ports.push_back(curComp);
        }
        return ports;
}

QStringList ModClassTree::getPorts(ModClass* parent,Modelica::NameFormat format)
{
        QList<ModClass*> _ports = getPorts(parent);

        QStringList _portsNames;
        for(int i=0;i<_ports.size();i++)
                _portsNames.push_back(_ports.at(i)->name(format));

        return _portsNames;
}


QIcon ModClassTree::getModelicaNodeIcon(ModClass* modClass)
{
    //    switch (modClass->getClassRestr())
    //    {
    //    case Modelica::MODEL
    //    case StringHandler::MODEL:
    //        return QIcon(":/Resources/icons/model-icon.png");
    //    case StringHandler::CLASS:
    //        return QIcon(":/Resources/icons/class-icon.png");
    //    case StringHandler::CONNECTOR:
    //        return QIcon(":/Resources/icons/connector-icon.png");
    //    case StringHandler::RECORD:
    //        return QIcon(":/Resources/icons/record-icon.png");
    //    case StringHandler::BLOCK:
    //        return QIcon(":/Resources/icons/block-icon.png");
    //    case StringHandler::FUNCTION:
    //        return QIcon(":/Resources/icons/function-icon.png");
    //    case StringHandler::PACKAGE:
    //        return QIcon(":/Resources/icons/package-icon.png");
    //    case StringHandler::TYPE:
    //        return QIcon(":/Resources/icons/type-icon.png");
    //    }
    return QIcon();
}

bool ModClassTree::canFetchMore ( const QModelIndex & parent ) const
{
    if(!_enabled)
        return false;

    if (!parent.isValid())
        return false;

    if(parent.column()<0 || parent.column()>=ModClass::nbFields)
        return false;

    ModClass *item = static_cast<ModClass*>(parent.internalPointer());

    if(item && !item->childrenReaden())
        return true;
    else
        return false;
}

void ModClassTree::fetchMore ( const QModelIndex & parent )
{
    if(!_enabled)
        return;

    if (!parent.isValid())
        return;

    ModClass *item = static_cast<ModClass*>(parent.internalPointer());

    if(item)
    {
        if(!item->childrenReaden())
            readFromOmcV2(item,_modReader->getDepthMax());
    }
}
