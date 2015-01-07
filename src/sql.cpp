/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "sql.h"
#include "phantom.h"
#include "config.h"
#include "cookiejar.h"
#include "terminal.h"


// FileSystem
// public:
Sql::Sql(QObject *parent)
    : QObject(parent)
    , m_db(0)
{
    m_value = "toto";
}

// public slots:
QString Sql::coucou() const
{
    return m_value;
}


bool Sql::_connect(const QVariantMap &config)
{
   if (m_db) {
        m_db->close();
    }

    if (config.contains("driver")) qDebug() << config["driver"].toString();
    if (config.contains("host")) qDebug() << config["host"].toString();
    if (config.contains("database")) qDebug() << config["database"].toString();
    if (config.contains("user")) qDebug() << config["user"].toString();
    if (config.contains("password")) qDebug() << config["password"].toString();
    if (config.contains("flags"))

    qDebug() << qApp->libraryPaths();

    //QSqlDatabase db = QSqlDatabase::addDatabase (dataBase);


    m_db = new QSqlDatabase(QSqlDatabase::addDatabase (config.value("driver").toString()));
    /*m_db->addDatabase(dataBase);*/
    if (config.contains("host")) m_db->setHostName(config.value("host").toString());
    if (config.contains("database")) m_db->setDatabaseName(config.value("database").toString());
    if (config.contains("user")) m_db->setUserName(config.value("user").toString());
    if (config.contains("password")) m_db->setPassword(config.value("password").toString());
    if (config.contains("flags")) {
        QStringList list = config.value("flags").toString().split(",", QString::SkipEmptyParts);
        QStringList::Iterator it = list.begin();
        QString flags = "CLIENT_"+*it;++it;
        for ( ; it != list.end(); ++it ) {
            flags += ";CLIENT_"+ *it ;
        }
        qDebug() << flags;
        m_db->setConnectOptions(flags);
    }

    return true;
}

bool Sql::open()
{
    QVariantMap data;

    if (!m_db->open()){
        data["stack"]=m_db->lastError().text();
        emit connected(data);
        m_db->setConnectOptions();
        return false;
    }
    emit connected(data);
    return true;
}


QString Sql::_query(const QString &request,const QVariantMap &bindValues)
{
    QList <QVariant> rows;
    QList <QVariant> cols;
    QVariantMap data;
    QVariantMap::ConstIterator ii;

    QString str;

    if (m_db->open()) {

        QSqlQuery qry;
        qry.prepare( request );

        for( ii = bindValues.constBegin(); ii != bindValues.constEnd(); ++ii ){
            qry.bindValue (ii.key(),ii.value());
        }

        if( !qry.exec() ){
              qDebug() << qry.lastError().text();
        } else {
            if (qry.isSelect()) { // SELECT COMMAND

                qDebug() << "Selected!" ;
                QSqlRecord rec = qry.record();
                int nbcols = rec.count();
                data["nbFields"] = nbcols;

                for( int c=0; c<nbcols; c++ ){
                    qDebug() << QString( "Column %1: %2" ).arg( c ).arg( rec.fieldName(c) );
                    str.append(QString("%1").arg(c));
                    cols << rec.fieldName(c);
                }

                data["nbRows"] = qry.size();

                for( int r=0; qry.next(); r++ ){
                    QList <QVariant> row;
                    for( int c=0; c<nbcols; c++ ){

                        qDebug() << QString( "Row %1, %2: %3" ).arg( r ).arg( rec.fieldName(c) ).arg( qry.value(c).toString() );
                        row << QVariant(qry.value(c).toString());
                    }
                    rows << row;
                }
                data["rows"]=QVariant(rows);
                data["cols"]=cols;
            }else{
                data["affectedRows"] = qry.numRowsAffected();
                data["insertId"] = qry.lastInsertId();
            }
            data["sql"] = qry.lastQuery();
        }
    }
    emit rowReceived(data);
    return "no-no";
}

bool Sql::close()
{
    if (m_db) {
        m_db->close();
    }
    return true;
}
