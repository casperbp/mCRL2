// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "filesystem.h"
#include "addeditpropertydialog.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QDirIterator>
#include <QMessageBox>
#include <QInputDialog>

Property::Property(QString name, QString text)
{
  this->name = name;
  this->text = text;
}

bool Property::equals(Property* property)
{
  return this->name == property->name && this->text == property->text;
}

Project::Project(QString projectName, std::list<Property*> properties)
{
  this->projectName = projectName;
  this->properties = properties;
}

FileSystem::FileSystem(CodeEditor* specificationEditor, QWidget* parent)
{
  this->specificationEditor = specificationEditor;
  this->parent = parent;
  specificationModified = false;
  connect(specificationEditor, SIGNAL(textChanged()), this,
          SLOT(setSpecificationModified()));

  projectOpen = false;

  makeSureProjectsFolderExists();
}

void FileSystem::makeSureProjectsFolderExists()
{
  if (!QDir(projectsFolderPath).exists())
  {
    QDir().mkpath(projectsFolderPath);
  }
}

void FileSystem::makeSurePropertiesFolderExists()
{
  if (!QDir(propertiesFolderPath()).exists())
  {
    QDir().mkpath(propertiesFolderPath());
  }
}

QString FileSystem::projectFolderPath()
{
  return projectsFolderPath + QDir::separator() + projectName;
}

QString FileSystem::propertiesFolderPath()
{
  return projectFolderPath() + QDir::separator() +
         propertiesFolderName;
}

QString FileSystem::specificationFilePath()
{
  return projectFolderPath() + QDir::separator() + projectName +
         "_spec.mcrl";
}

QString FileSystem::lpsFilePath()
{
  return projectFolderPath() + QDir::separator() + projectName +
         "_lps.lps";
}

QString FileSystem::ltsFilePath(LtsReduction reduction)
{
  return projectFolderPath() + QDir::separator() + projectName +
         "_lts_" + LTSREDUCTIONNAMES.at(reduction) + ".lts";
}

QString FileSystem::propertyFilePath(QString propertyName)
{
  return propertiesFolderPath() + QDir::separator() + propertyName +
         ".mcf";
}

QString FileSystem::pbesFilePath(QString propertyName)
{
  return propertiesFolderPath() + QDir::separator() + projectName +
         "_" + propertyName + "_pbes.pbes";
}

bool FileSystem::projectOpened()
{
  return projectOpen;
}

QString FileSystem::getCurrentSpecification()
{
  return specificationEditor->toPlainText();
}

bool FileSystem::isSpecificationModified()
{
  return specificationModified;
}

void FileSystem::setSpecificationModified()
{
  specificationModified = true;
  emit hasChanges(true);
}

bool FileSystem::propertyNameExists(QString propertyName)
{
  for (Property* property : properties)
  {
    if (property->name == propertyName)
    {
      return true;
    }
  }
  return false;
}

bool FileSystem::upToDateLpsFileExists()
{
  /* an lps file is up to date if the lps file exists and the lps file is
   *   created after the the last time the specification file was modified */
  return QFile(lpsFilePath()).exists() &&
         QFileInfo(specificationFilePath()).lastModified() <=
             QFileInfo(lpsFilePath()).lastModified();
}

bool FileSystem::upToDateLtsFileExists(LtsReduction reduction)
{
  /* in case not reduced, an lts file is up to date if the lts file exists and
   *   the lts file is created after the the last time the lps file was modified
   * in case reduced, an lts file is up to date if the lts file exists and
   *   the lts file is created after the the last time the unreduced lts file
   *   was modified */
  if (reduction == LtsReduction::None)
  {
    return QFile(ltsFilePath(reduction)).exists() &&
           QFileInfo(lpsFilePath()).lastModified() <=
               QFileInfo(ltsFilePath(reduction)).lastModified();
  }
  else
  {
    return QFile(ltsFilePath(reduction)).exists() &&
           QFileInfo(ltsFilePath(LtsReduction::None)).lastModified() <=
               QFileInfo(ltsFilePath(reduction)).lastModified();
  }
}

bool FileSystem::upToDatePbesFileExists(QString propertyName)
{
  /* a pbes file is up to date if the pbes file exists and the pbes file is
   *   created after the last time both the lps and the property files were
   *   modified */
  return QFile(pbesFilePath(propertyName)).exists() &&
         QFileInfo(lpsFilePath()).lastModified() <=
             QFileInfo(pbesFilePath(propertyName)).lastModified() &&
         QFileInfo(propertyFilePath(propertyName)).lastModified() <=
             QFileInfo(pbesFilePath(propertyName)).lastModified();
}

void FileSystem::setSpecificationEditorCursor(int row, int column)
{
  QTextCursor cursor = specificationEditor->textCursor();
  cursor.movePosition(QTextCursor::Start);
  cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, row - 1);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
  specificationEditor->setTextCursor(cursor);
}

QString FileSystem::newProject(QString context)
{
  makeSureProjectsFolderExists();

  QString error = "";

  /* ask the user for a project name */
  bool ok;
  QString projectName =
      QInputDialog::getText(parent, context, "Project name:", QLineEdit::Normal,
                            "", &ok, Qt::WindowCloseButtonHint);

  /* if user pressed ok, create the project and save the specification and
   *   properties in it */
  if (ok)
  {
    /* the project name may not be empty */
    if (projectName.isEmpty())
    {
      error = "The project name may not be empty";
    }
    else
    {
      /* create the folder for this project */
      if (QDir(projectsFolderPath).mkdir(projectName))
      {
        /* if successful, create the properties folder too */
        this->projectName = projectName;
        QDir(projectFolderPath()).mkdir(propertiesFolderName);
        projectOpen = true;
      }
      else
      {
        /* if unsuccessful, there already is a project folder with this name */
        error = "A project with this name already exists";
      }
    }
  }

  if (error.isEmpty())
  {
    return projectName;
  }
  else
  {
    /* if there was an error, tell the user */
    QMessageBox* msgBox =
        new QMessageBox(QMessageBox::Information, context, error,
                        QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox->exec();
    delete msgBox;
    return "";
  }
}

bool FileSystem::deletePropertyFile(QString propertyName, bool showIfFailed)
{
  QFile* propertyFile = new QFile(propertyFilePath(propertyName));
  bool deleteSucceeded = true;
  if (propertyFile->exists() && !propertyFile->remove())
  {
    /* if deleting the file failed, tell the user */
    QMessageBox* msgBox = new QMessageBox(
        QMessageBox::Information, "Delete property",
        "Could not delete property file: " + propertyFile->errorString(),
        QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox->exec();
    deleteSucceeded = false;
    delete msgBox;
  }

  delete propertyFile;
  return deleteSucceeded;
}

void FileSystem::deleteUnlistedPropertyFiles()
{
  QDir propertiesFolder = QDir(propertiesFolderPath());
  QStringList fileNames = propertiesFolder.entryList();
  std::list<QString> testl = fileNames.toStdList();
  for (QString fileName : fileNames)
  {
    std::string test1 = fileName.toStdString();
    if (fileName.endsWith(".mcf"))
    {
      QString propertyName = fileName.left(fileName.length() - 4);
      std::string test2 = propertyName.toStdString();
      if (!propertyNameExists(propertyName))
      {
        deletePropertyFile(propertyName, false);
      }
    }
  }
}

Property* FileSystem::newProperty(ProcessSystem* processSystem)
{
  makeSurePropertiesFolderExists();

  Property* property = NULL;
  AddEditPropertyDialog* addPropertyDialog =
      new AddEditPropertyDialog(true, processSystem, this, parent);

  /* if successful (Add button was pressed), create the new property
   * we don't need to save to file as this is already done by the dialog */
  if (addPropertyDialog->exec())
  {
    property = addPropertyDialog->getProperty();
    properties.push_back(property);
  }
  /* delete any obsolete property files generated by the dialog */
  deleteUnlistedPropertyFiles();

  delete addPropertyDialog;
  return property;
}

Property* FileSystem::editProperty(Property* oldProperty,
                                   ProcessSystem* processSystem)
{
  makeSurePropertiesFolderExists();

  Property* newProperty = oldProperty;
  AddEditPropertyDialog* editPropertyDialog = new AddEditPropertyDialog(
      false, processSystem, this, parent, oldProperty->name, oldProperty->text);

  /* if editing was successful (Edit button was pressed), change the property
   * we don't need to save to file as this is already done by the dialog */
  if (editPropertyDialog->exec())
  {
    newProperty = editPropertyDialog->getProperty();

    /* alter the properties list */
    for (Property* property : properties)
    {
      if (property->equals(oldProperty))
      {
        *property = *newProperty;
        break;
      }
    }
  }
  /* delete any obsolete property files generated by the dialog
   * this also deletes the original property file if the property name has
   *   changed */
  deleteUnlistedPropertyFiles();

  delete editPropertyDialog;
  return newProperty;
}

bool FileSystem::deleteProperty(Property* oldProperty)
{
  /* show a message box to ask the user whether he is sure to delete the
   *   property */
  bool deleteIt =
      QMessageBox::question(parent, "Delete Property",
                            "Are you sure you want to delete the property " +
                                oldProperty->name + "?",
                            QMessageBox::Yes | QMessageBox::Cancel) ==
      QMessageBox::Yes;

  /* if the user agrees, delete the property */
  if (deleteIt)
  {
    Property* toRemove = NULL;
    for (Property* property : properties)
    {
      if (property->equals(oldProperty))
      {
        toRemove = property;
        break;
      }
    }
    if (toRemove != NULL)
    {
      properties.remove(toRemove);
    }

    /* also delete the file if it exists */
    if (!deletePropertyFile(oldProperty->name))
    {
      deleteIt = false;
    }
  }

  return deleteIt;
}

Project FileSystem::openProject()
{
  makeSureProjectsFolderExists();

  QStringList projects =
      QDir(projectsFolderPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  Project project = Project("", {});

  /* if there are no projects give an error, else ask a project name from the
   *   user */
  if (projects.isEmpty())
  {
    QMessageBox* msgBox = new QMessageBox(
        QMessageBox::Information, "Open Project", "No projects found",
        QMessageBox::Ok, parent, Qt::WindowCloseButtonHint);
    msgBox->exec();
    delete msgBox;
  }
  else
  {
    bool ok;
    QString projectName =
        QInputDialog::getItem(parent, "Open Project", "Project name:", projects,
                              0, false, &ok, Qt::WindowCloseButtonHint);

    /* if user pressed ok, open the project */
    if (ok)
    {
      this->projectName = projectName;
      projectOpen = true;

      /* read the specification and put it in the specification editor */
      QFile* specificationFile = new QFile(specificationFilePath());
      specificationFile->open(QIODevice::ReadOnly);
      QTextStream* openStream = new QTextStream(specificationFile);
      QString spec = openStream->readAll();
      specificationEditor->setPlainText(spec);
      specificationFile->close();
      delete specificationFile;
      delete openStream;

      /* read all properties */
      properties.clear();
      QDirIterator* dirIterator =
          new QDirIterator(QDir(propertiesFolderPath()));

      while (dirIterator->hasNext())
      {
        QFile* propertyFile = new QFile(dirIterator->next());
        QFileInfo* propertyFileInfo = new QFileInfo(*propertyFile);
        QString fileName = propertyFileInfo->fileName();

        if (propertyFileInfo->isFile() && fileName.endsWith(".mcf"))
        {
          fileName.chop(4);
          propertyFile->open(QIODevice::ReadOnly);
          QTextStream* openStream = new QTextStream(propertyFile);
          QString propertyText = openStream->readAll();
          properties.push_back(new Property(fileName, propertyText));
          propertyFile->close();
          delete openStream;
        }

        delete propertyFile;
        delete propertyFileInfo;
      }

      this->properties = properties;
      project = Project(projectName, properties);
      emit hasChanges(false);
      delete dirIterator;
    }
  }
  return project;
}

QString FileSystem::saveProject(bool forceSave)
{
  makeSurePropertiesFolderExists();

  /* if we have a project open, we have a location to save in so we can simply
   *   save, else use save as */
  if (projectOpen)
  {
    /* save the specification (when there are changes) */
    if (isSpecificationModified() || forceSave)
    {
      QFile* specificationFile = new QFile(specificationFilePath());
      specificationFile->open(QIODevice::WriteOnly);
      QTextStream* saveStream = new QTextStream(specificationFile);
      *saveStream << specificationEditor->toPlainText();
      specificationFile->close();
      specificationModified = false;
      delete specificationFile;
      delete saveStream;
    }

    /* save all properties if necessary */
    if (forceSave)
    {
      for (Property* property : properties)
      {
        saveProperty(property);
      }
    }

    emit hasChanges(false);
    return projectName;
  }
  else
  {
    return saveProjectAs();
  }
}

QString FileSystem::saveProjectAs()
{
  /* ask the user for a new project name */
  QString projectName = newProject("Save Project As");

  /* save if successful, else return the empty string */
  if (projectName.isEmpty())
  {
    return "";
  }
  else
  {
    saveProject(true);
    return projectName;
  }
}

void FileSystem::saveProperty(Property* property)
{
  makeSurePropertiesFolderExists();

  QFile* propertyFile = new QFile(propertyFilePath(property->name));
  propertyFile->open(QIODevice::WriteOnly);
  QTextStream* saveStream = new QTextStream(propertyFile);
  *saveStream << property->text;
  propertyFile->close();
  propertyModified[property->name] = false;
  delete propertyFile;
  delete saveStream;
}