/*jslint sloppy: true, nomen: true */
/*global exports:true,phantom:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com
  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

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

function defineSqlSignalHandler(sql, handlers, handlerName, signalName) {
    sql.__defineSetter__(handlerName, function (f) {
        // Disconnect previous handler (if any)
        if (!!handlers[handlerName] && typeof handlers[handlerName].callback === "function") {
            try {
                this[signalName].disconnect(handlers[handlerName].callback);
            } catch (e) {}
        }

        // Delete the previous handler
        delete handlers[handlerName];

        // Connect the new handler iff it's a function
        if (typeof f === "function") {
            // Store the new handler for reference
            handlers[handlerName] = {
                callback: f
            }
            this[signalName].connect(f);
        }
    });

    sql.__defineGetter__(handlerName, function() {
        return !!handlers[handlerName] && typeof handlers[handlerName].callback === "function" ?
            handlers[handlerName].callback :
            undefined;
    });
}

function isArray(object)
{
    if (object.constructor === Array) return true;
    else return false;
}

function decorateNewSql(opts, sql) {
    var handlers = {};
    opts['driver'] = 'QMYSQL';

    defineSqlSignalHandler(sql, handlers, "_onRowReceived", "rowReceived");
    defineSqlSignalHandler(sql, handlers, "_onConnected", "connected")

    sql._connect(opts);

    sql.connect =function(arg1){
                thisSql = this;

                thisSql._onConnected = function(err) {
                   thisSql._onConnected = null; //< Disconnect callback (should fire only once)
                    if (err && typeof err.stack !== "undefined"){
                        arg1.call(thisSql, err);        //< Invoke the actual callback
                    } else {
                        arg1.call(thisSql);
                    }

                };
                return thisSql.open();
    };

    sql.query = function(req,arg1,arg2){
        thisSql = this;
        nbArg = arguments.length;
        arg2 = arguments[nbArg-1];
        param = {};

        if (nbArg >= 2 && typeof arg2 === 'function') {
            thisSql._onRowReceived = function(data) {
                thisSql._onRowReceived = null; //< Disconnect callback (should fire only once)
                arg2.call(thisSql, data);        //< Invoke the actual callback
            };
        }
        if (arg1 && isArray(arg1)) {
                for (i=0,l=arg1.length;i<l;i++)
                param[i]=arg1[i];
        } else if (arg1 && typeof arg1 === 'object'){
                param = arg1;
        } else if (arg1 && typeof arg1 !== 'function'){
            param = {"0":arg1};
        }

        this._query(req,param);
        return;
    };

    sql.end = function(){
                sql.close();
    };

    return sql;
}

exports.createConnection = function (opts) {
    return decorateNewSql(opts, phantom.createSql());
};
