// Copyright (c) 2019 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "createmoduleprocess.h"

CreateModuleProcess::CreateModuleProcess(QObject *parent) : QObject(parent)
{
    bIsStop=false;
    currentStats={};
}

void CreateModuleProcess::setData(Utils::MDATA *pMData)
{
    this->pMData=pMData;
}

void CreateModuleProcess::stop()
{
    bIsStop=true;
}

CreateModuleProcess::STATS CreateModuleProcess::getCurrentStats()
{
    return currentStats;
}

QByteArray CreateModuleProcess::createPluginInfo(Utils::MDATA *pMData, QList<Utils::FILE_RECORD> *pListFileRecords, QList<Utils::DIRECTORY_RECORD> *pListDirectoryRecords)
{
    QByteArray baResult;

    int nFilesCount=pListFileRecords->count();
    int nDirectoriesCount=pListDirectoryRecords->count();

    QJsonObject recordObject;
    recordObject.insert("Name",             QJsonValue::fromVariant(pMData->sName));
    recordObject.insert("Version",          QJsonValue::fromVariant(pMData->sVersion));
    recordObject.insert("Date",             QJsonValue::fromVariant(pMData->sDate));
    recordObject.insert("Info",             QJsonValue::fromVariant(pMData->sInfo));
    recordObject.insert("Size",             QJsonValue::fromVariant(pMData->nSize));
    recordObject.insert("CompressedSize",   QJsonValue::fromVariant(pMData->nCompressedSize));

    QJsonArray installArray;

    for(int i=0;i<nDirectoriesCount;i++)
    {
        QJsonObject record;

        record.insert("Path",       pListDirectoryRecords->at(i).sPath);
        record.insert("Action",     "make_directory");

        installArray.append(record);
    }

    for(int i=0;i<nFilesCount;i++)
    {
        QJsonObject record;

        record.insert("Path",       pListFileRecords->at(i).sPath);
        record.insert("Action",     "copy_file");
        record.insert("SHA1",       pListFileRecords->at(i).sSHA1);

        installArray.append(record);
    }

    recordObject.insert("Install",  installArray);

    QJsonArray removeArray;

    for(int i=0;i<nFilesCount;i++)
    {
        QJsonObject record;

        record.insert("Path",       pListFileRecords->at(i).sPath);
        record.insert("Action",     "remove_file");

        removeArray.append(record);
    }

    for(int i=0;i<nDirectoriesCount;i++)
    {
        QJsonObject record;

        record.insert("Path",       pListDirectoryRecords->at(i).sPath);
        record.insert("Action",     "remove_directory_if_empty");

        removeArray.append(record);
    }

    recordObject.insert("Remove",   removeArray);

    QJsonDocument doc(recordObject);
    baResult.append(doc.toJson());

    return baResult;
}

void CreateModuleProcess::process()
{
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    bIsStop=false;

    QList<Utils::FILE_RECORD> listFileRecords;
    QList<Utils::DIRECTORY_RECORD> listDirectoryRecords;

    currentStats.nTotal=pMData->listRecords.count();

    QString sBundleFileName=pMData->sBundlePath+QDir::separator()+pMData->sBundleName+".x64dbg.zip";

    bool bSuccess=true;

    if(XBinary::isFileExists(sBundleFileName))
    {
        bSuccess=XBinary::removeFile(sBundleFileName);

        if(!bSuccess)
        {
            emit errorMessage(tr("Cannot remove: %1").arg(sBundleFileName));
        }
    }

    pMData->nSize=0;
    pMData->nCompressedSize=0;

    if(bSuccess)
    {
        QFile fileResult;

        fileResult.setFileName(sBundleFileName);

        if(fileResult.open(QIODevice::ReadWrite))
        {
            QList<XZip::ZIPFILE_RECORD> listZipFiles;

            for(int i=0;(i<currentStats.nTotal)&&(!bIsStop);i++)
            {       
                if(pMData->listRecords.at(i).bIsFile)
                {
                    currentStats.sStatus=QString("%1: %2").arg(tr("Add file")).arg(pMData->listRecords.at(i).sPath);

                    QFile file;

                    file.setFileName(pMData->listRecords.at(i).sFullPath);

                    if(file.open(QIODevice::ReadOnly))
                    {
                        Utils::FILE_RECORD fileRecord={};

                        fileRecord.sFullPath=pMData->listRecords.at(i).sFullPath;
                        fileRecord.sPath=pMData->listRecords.at(i).sPath;
                        fileRecord.sSHA1=XBinary::getHash(XBinary::HASH_SHA1,&file);

                        XZip::ZIPFILE_RECORD zipFileRecord={};

                        zipFileRecord.sFileName=QString("files/")+fileRecord.sPath;
                        zipFileRecord.method=XZip::METHOD_DEFLATE;

                        XZip::addLocalFileRecord(&file,&fileResult,&zipFileRecord); // TODO handle errors

                        pMData->nSize+=zipFileRecord.nUncompressedSize;
                        pMData->nCompressedSize+=zipFileRecord.nUncompressedSize;

                        file.close();

                        listFileRecords.append(fileRecord);
                        listZipFiles.append(zipFileRecord);
                    }
                }
                else
                {
                    currentStats.sStatus=QString("%1: %2").arg(tr("Add directory")).arg(pMData->listRecords.at(i).sPath);

                    Utils::DIRECTORY_RECORD directoryRecord={};

                    directoryRecord.sFullPath=pMData->listRecords.at(i).sFullPath;
                    directoryRecord.sPath=pMData->listRecords.at(i).sPath;

                    listDirectoryRecords.append(directoryRecord);
                }

                currentStats.nCurrent=i+1;
            }

            // TODO info file
            QByteArray baInfoFile=createPluginInfo(pMData,&listFileRecords,&listDirectoryRecords);

            QBuffer bufferInfoFile(&baInfoFile);

            if(bufferInfoFile.open(QIODevice::ReadOnly))
            {
                XZip::ZIPFILE_RECORD zipFileRecord={};

                zipFileRecord.sFileName="plugin_info.json";
                zipFileRecord.method=XZip::METHOD_DEFLATE;

                XZip::addLocalFileRecord(&bufferInfoFile,&fileResult,&zipFileRecord);

                pMData->nSize+=zipFileRecord.nUncompressedSize;
                pMData->nCompressedSize+=zipFileRecord.nUncompressedSize;

                listZipFiles.append(zipFileRecord);

                bufferInfoFile.close();
            }

            if(!bIsStop)
            {
                currentStats.sStatus=tr("Add central directory");

                XZip::addCentralDirectory(&fileResult,&listZipFiles,QString("%1 v%2").arg(X_APPLICATIONNAME).arg(X_APPLICATIONVERSION));
            }

            // TODO

            fileResult.close();
        }
    }

    // TODO
    emit completed(elapsedTimer.elapsed());
}
