/*
 * The contents of this file are subject to the MonetDB Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at
 * http://monetdb.cwi.nl/Legal/MonetDBLicense-1.0.html
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Monet Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-2005 CWI.
 * All Rights Reserved.
 */

package nl.cwi.monetdb.jdbc;

import java.sql.*;
import java.io.*;
import java.util.*;

/**
 * A Statement suitable for the MonetDB database.
 * <br /><br />
 * The object used for executing a static SQL statement and returning the
 * results it produces.<br />
 * <br /><br />
 * By default, only one ResultSet object per Statement object can be open at the
 * same time. Therefore, if the reading of one ResultSet object is interleaved
 * with the reading of another, each must have been generated by different
 * Statement objects. All execution methods in the Statement interface implicitly
 * close a Statement's current ResultSet object if an open one exists.
 * <br /><br />
 * The current state of this Statement is that it only implements the
 * executeQuery() which returns a ResultSet where from results can be read
 * and executeUpdate() which doesn't return the affected rows. Commit and
 * rollback are implemented, as is the autoCommit mechanism which relies on
 * server side auto commit.<br />
 * Multi-result queries are supported using the getMoreResults() method.
 *
 * @author Fabian Groffen <Fabian.Groffen@cwi.nl>
 * @version 0.6
 */
public class MonetStatement implements Statement {
	/** the default value of maxRows, 0 indicates unlimited */
	static final int DEF_MAXROWS = 0;

	/** The parental Connection object */
	private MonetConnection connection;
	/** The last HeaderList object this Statement produced */
	private MonetConnection.HeaderList lastHeaderList;
	/** The last Header that this object uses */
	private MonetConnection.Header header;
	/** The warnings this Statement object generated */
	private SQLWarning warnings;
	/** Whether this Statement object is closed or not */
	private boolean closed;
	/** The size of the blocks of results to ask for at the server */
	private int fetchSize = 0;
	/** The maximum number of rows to return in a ResultSet */
	private int maxRows = DEF_MAXROWS;
	/** The suggested direction of fetching data (implemented but not used) */
	private int fetchDirection = ResultSet.FETCH_FORWARD;
	/** The type of ResultSet to produce; i.e. forward only, random access */
	private int resultSetType = ResultSet.TYPE_FORWARD_ONLY;
	/** The concurrency of the ResultSet to produce */
	private int resultSetConcurrency = ResultSet.CONCUR_READ_ONLY;

	/** A StringBuffer to hold all queries of a batch */
	private StringBuffer batch;

	/** Query types (copied from sql_query.mx) */
	final static int Q_END = 0;
	final static int Q_PARSE = 1;
	final static int Q_RESULT = 2;
	final static int Q_TABLE = 3;
	final static int Q_UPDATE = 4;
	final static int Q_DATA = 5;
	final static int Q_SCHEMA = 6;
	final static int Q_TRANS = 7;
	final static int Q_DEBUG = 8;
	final static int Q_DEBUGP = 9;


	/**
	 * MonetStatement constructor which checks the arguments for validity, tries
	 * to set up a socket to MonetDB and attempts to login.
	 * This constructor is only accessible to classes from the jdbc package.
	 *
	 * @param connection the connection that created this Statement
	 * @param resultSetType type of ResultSet to produce
	 * @param resultSetConcurrency concurrency of ResultSet to produce
	 * @throws SQLException if an error occurs during login
	 * @throws IllegalArgumentException is one of the arguments is null or empty
	 */
	MonetStatement(
		MonetConnection connection,
		int resultSetType,
		int resultSetConcurrency)
		throws SQLException, IllegalArgumentException
	{
		if (connection == null) throw
			new IllegalArgumentException("No Connection given!");

		this.connection = connection;
		this.resultSetType = resultSetType;
		this.resultSetConcurrency = resultSetConcurrency;

		// check our limits, and generate warnings as appropriate
		if (resultSetConcurrency != ResultSet.CONCUR_READ_ONLY) {
			addWarning("No concurrency mode other then read only is supported, continuing with concurrency level READ_ONLY");
			resultSetConcurrency = ResultSet.CONCUR_READ_ONLY;
		}

		// check type for supported mode
		if (resultSetType == ResultSet.TYPE_SCROLL_SENSITIVE) {
			addWarning("Change sensitive scrolling ResultSet objects are not supported, continuing with a change non-sensitive scrollable cursor.");
			resultSetType = ResultSet.TYPE_SCROLL_INSENSITIVE;
		}

		closed = false;
	}

	//== methods of interface Statement

	/**
	 * Adds the given SQL command to the current list of commmands for this
	 * Statement object.  The commands in this list can be executed as a
	 * batch by calling the method executeBatch.
	 *
	 * @param sql typically this is a static SQL INSERT or UPDATE statement
	 * @throws SQLException so the PreparedStatement can throw this exception
	 */
	public void addBatch(String sql) throws SQLException {
		// we assume a batch is big; average chars per q = 60; average q = 1000
		if (batch == null) batch = new StringBuffer(60000);

		// save compiler some work :) `sql + ";"' will be converted to:
		// new StringBuffer(sql).append(";")
		batch.append(sql).append(';').append('\n');
	}

	/**
	 * Empties this Statement object's current list of SQL commands.
	 */
	public void clearBatch() {
		// we simply feed the `old' StringBuffer to the garbage collector
		batch = null;
	}

	/**
	 * Submits a batch of commands to the database for execution and if all
	 * commands execute successfully, returns an array of update counts.  The
	 * int elements of the array that is returned are ordered to correspond
	 * to the commands in the batch, which are ordered according to the order
	 * in which they were added to the batch.  The elements in the array
	 * returned by the method executeBatch may be one of the following:
	 * <br />
	 * <ol>
	 * <li>A number greater than or equal to zero -- indicates that the command
	 * was processed successfully and is an update count giving the number of
	 * rows in the database that were affected by the command's execution</li>
	 * <li>A value of SUCCESS_NO_INFO -- indicates that the command was
	 * processed successfully but that the number of rows affected is unknown
	 * </li>
	 * If one of the commands in a batch update fails to execute properly, this
	 * method throws a BatchUpdateException, and a JDBC driver may or may not
	 * continue to process the remaining commands in the batch.  However, the
	 * driver's behavior must be consistent with a particular DBMS, either
	 * always continuing to process commands or never continuing to process
	 * commands.
	 * <br /><br />
	 * MonetDB does not continue after an error has occurred in the batch. If
	 * one of the commands attempts to return a result set, a warning will be
	 * added to the Statements warnings list.  This behaviour is chosen because
	 * the batch will not be cancelled if it doesn't contain errors. <br />
	 * <b>NOTE: this behaviour does not comply with the interface, where an
	 * BatchUpdateException should have been thrown.</b>
	 *
	 * @return an array of update counts containing one element for each
	 *         command in the batch.  The elements of the array are ordered
	 *         according to the order in which commands were added to the
	 *         batch.
	 * @throws SQLException if a database access error occurs.  Throws
	 *         BatchUpdateException (a subclass of SQLException) if one of the
	 *         commands sent to the database fails to execute properly
	 */
	public synchronized int[] executeBatch() throws SQLException {
		// this method is synchronized to make sure noone gets inbetween the
		// operations we execute below

		List counts = new ArrayList();
		String error = null;

		if (batch != null) {
			try {
				boolean type = internalExecute(batch.toString());
				int count = -1;
				if (!type) count = getUpdateCount();
				do {
					if (type) {
						addWarning("Batch query produced a ResultSet!  Ignoring, " +
								"setting update count to value " + SUCCESS_NO_INFO);
						counts.add(new Integer(SUCCESS_NO_INFO));
					} else if (count >= 0) {
						counts.add(new Integer(count));
					}
				} while ((type = getMoreResults()) ||
						(count = getUpdateCount()) != -1);
			} catch (SQLException e) {
				error = e.getMessage();
			}
		}

		int[] ret = new int[counts.size()];
		for (int i = 0; i < counts.size(); i++) {
			ret[i] = ((Integer)(counts.get(i))).intValue();
		}

		if (error != null) {
			throw new BatchUpdateException(error, ret);
		}

		return(ret);
	}

	/**
     * Cancels this Statement object if both the DBMS and driver support
	 * aborting an SQL statement.  This method can be used by one thread to
	 * cancel a statement that is being executed by another thread.
	 *
	 * @throws SQLException if a database access error occurs or the cancel
	 *                      operation is not supported
	 */
	public void cancel() throws SQLException {
		throw new SQLException("Query cancelling is currently not supported by the DBMS.");
	}

	/**
	 * Clears all warnings reported for this Statement object. After a call to
	 * this method, the method getWarnings returns null until a new warning is
	 * reported for this Statement object.
	 */
	public void clearWarnings() {
		warnings = null;
	}

	/**
	 * Releases this Statement object's database and JDBC resources immediately
	 * instead of waiting for this to happen when it is automatically closed. It
	 * is generally good practice to release resources as soon as you are
	 * finished with them to avoid tying up database resources.
	 * <br /><br />
	 * Calling the method close on a Statement object that is already closed has
	 * no effect.
	 * <br /><br />
	 * A Statement object is automatically closed when it is garbage collected.
	 * When a Statement object is closed, its current ResultSet object, if one
	 * exists, is also closed.
	 */
	public void close() {
		// close previous ResultSet, if not closed already
		if (lastHeaderList != null) lastHeaderList.close();
		closed = true;
	}

	// Chapter 13.1.2.3 of Sun's JDBC 3.0 Specification
	/**
	 * Executes the given SQL statement, which may return multiple results. In
	 * some (uncommon) situations, a single SQL statement may return multiple
	 * result sets and/or update counts. Normally you can ignore this unless
	 * you are (1) executing a stored procedure that you know may return
	 * multiple results or (2) you are dynamically executing an unknown SQL
	 * string.
	 * <br /><br />
	 * The execute method executes an SQL statement and indicates the form of
	 * the first result. You must then use the methods getResultSet or
	 * getUpdateCount to retrieve the result, and getMoreResults to move to any
	 * subsequent result(s).
	 *
	 * @param sql any SQL statement
	 * @return true if the first result is a ResultSet object; false if it is an
	 *         update count or there are no results
	 * @throws SQLException if a database access error occurs
	 */
	public boolean execute(String sql) throws SQLException {
		return(internalExecute(sql));
	}
	
	/**
	 * Performs the steps to execute a given SQL statement.  This method exists
	 * to allow the functionality of this function to be called from within
	 * this class only.  The PreparedStatement for example overrides the
	 * execute() method to throw an SQLException, but it needs its
	 * functionality when the executeBatch method (which is inherited) is
	 * called.
	 *
	 * @param sql any SQL statement
	 * @return true if the first result is a ResultSet object; false if it is an
	 *         update count or there are no results
	 * @throws SQLException if a database access error occurs
	 */
	private boolean internalExecute(String sql) throws SQLException {
		// close previous query, if not closed already
		if (lastHeaderList != null) {
			lastHeaderList.close();
			lastHeaderList = null;
		}

		// let the cache thread do it's work
		// use lowercase 's' in order to tell the server we don't want a
		// continuation prompt if it needs more to complete the query
		// always add ; since that doesn't hurt in any case and completes
		// queries that haven't got one
		lastHeaderList = connection.addQuery(
			sql,
			fetchSize,
			maxRows,
			resultSetType,
			resultSetConcurrency
		);
		// give the cache thread a chance to run before we continue blocking
		// for it...
		Thread.yield();

		return(getMoreResults());
	}

	public boolean execute(String sql, int autoGeneratedKeys)
		throws SQLException
	{
		throw new SQLException("Method not supported, sorry!");
	}
	public boolean execute(String sql, int[] columnIndexed)
		throws SQLException
	{
		throw new SQLException("Method not supported, sorry!");
	}
	public boolean execute(String sql, String[] columnNames)
		throws SQLException
	{
		throw new SQLException("Method not supported, sorry!");
	}

	/**
	 * Executes the given SQL statement, which returns a single ResultSet
	 * object.
	 *
	 * @param sql an SQL statement to be sent to the database, typically a
	 *        static SQL SELECT statement
	 * @return a ResultSet object that contains the data produced by the given
	 *         query; never null
	 * @throws SQLException if a database access error occurs or the given SQL
	 *         statement produces anything other than a single ResultSet object
	 */
	public ResultSet executeQuery(String sql) throws SQLException {
		if (execute(sql) != true)
			throw new SQLException("Query did not produce a result set");

		return(getResultSet());
	}

	/**
	 * Executes the given SQL statement, which may be an INSERT, UPDATE, or
	 * DELETE statement or an SQL statement that returns nothing, such as an
	 * SQL DDL statement.
	 * <br /><br />
	 * make an implementation which returns affected rows, need protocol
	 * modification for that!!!
	 *
	 * @param sql an SQL INSERT, UPDATE or DELETE statement or an SQL statement
	 *        that returns nothing
	 * @return either the row count for INSERT, UPDATE  or DELETE statements, or
	 *         0 for SQL statements that return nothing<br />
	 *         <b>currently always returns -1 since the mapi protocol doesn't
	 *         return the affected rows!!!</b>
	 * @throws SQLException if a database access error occurs or the given SQL
	 *         statement produces a ResultSet object
	 */
	public int executeUpdate(String sql) throws SQLException {
		if (execute(sql) != false)
			throw new SQLException("Query produced a result set");

		return(getUpdateCount());
	}

	public int executeUpdate(String sql, int autoGeneratedKeys) throws SQLException { throw new SQLException("Method not supported, sorry!"); }
	public int executeUpdate(String sql, int[] columnIndexed) throws SQLException { throw new SQLException("Method not supported, sorry!"); }
	public int executeUpdate(String sql, String[] columnNames) throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Retrieves the Connection object that produced this Statement object.
	 *
	 * @return the connection that produced this statement
	 */
	public Connection getConnection() {
		return(connection);
	}

	/**
	 * Retrieves the direction for fetching rows from database tables that is
	 * the default for result sets generated from this Statement object. If
	 * this Statement object has not set a fetch direction by calling the
	 * method setFetchDirection, the return value is ResultSet.FETCH_FORWARD.
	 *
	 * @return the default fetch direction for result sets generated from this
	 *         Statement object
	 */
	public int getFetchDirection() {
		return(fetchDirection);
	}

	/**
	 * Retrieves the number of result set rows that is the default fetch size
	 * for ResultSet objects generated from this Statement object. If this
	 * Statement object has not set a fetch size by calling the method
	 * setFetchSize, or the method setFetchSize was called as such to let
	 * the driver ignore the hint, 0 is returned.
	 *
	 * @return the default fetch size for result sets generated from this
	 *         Statement object
	 */
	public int getFetchSize() {
		return(fetchSize);
	}

	public ResultSet getGeneratedKeys() throws SQLException { throw new SQLException("Method not supported, sorry!"); }
	public int getMaxFieldSize() throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Retrieves the maximum number of rows that a ResultSet object produced by
	 * this Statement object can contain. If this limit is exceeded, the excess
	 * rows are silently dropped.
	 *
	 * @return the current maximum number of rows for a ResultSet object
	 *         produced by this Statement object; zero means there is no limit
	 */
	public int getMaxRows() {
		return(maxRows);
	}

	/**
	 * Moves to this Statement object's next result, returns true if it is a
	 * ResultSet object, and implicitly closes any current ResultSet object(s)
	 * obtained with the method getResultSet.
	 * <br /><br />
	 * There are no more results when the following is true:<br />
	 * (!getMoreResults() && (getUpdateCount() == -1)
	 *
	 * @return true if the next result is a ResultSet object; false if it is
	 *              an update count or there are no more results
	 * @throws SQLException if a database access error occurs
	 * @see #getMoreResults(int current)
	 */
	public boolean getMoreResults() throws SQLException {
		return(getMoreResults(CLOSE_ALL_RESULTS));
	}

	/**
	 * Moves to this Statement object's next result, deals with any current
	 * ResultSet object(s) according to the instructions specified by the given
	 * flag, and returns true if the next result is a ResultSet object.
	 * <br /><br />
	 * There are no more results when the following is true:<br />
	 * (!getMoreResults() && (getUpdateCount() == -1)
	 *
	 * @param current one of the following Statement constants indicating what
	 *                should happen to current ResultSet objects obtained using
	 *                the method getResultSet: CLOSE_CURRENT_RESULT,
	 *                KEEP_CURRENT_RESULT, or CLOSE_ALL_RESULTS
	 * @return true if the next result is a ResultSet object; false if it is
	 *              an update count or there are no more results
	 * @throws SQLException if a database access error occurs
	 */
	public boolean getMoreResults(int current) throws SQLException {
		if (current == CLOSE_CURRENT_RESULT) {
			lastHeaderList.closeCurrentHeader();
		} else if (current == CLOSE_ALL_RESULTS) {
			lastHeaderList.closeCurOldHeaders();
		}
		// we default to keep current result, which requires no action
		header = lastHeaderList.getNextHeader();

		if (header != null && header.getQueryType() == Q_TABLE) return(true);

		// no resultset, or update header
		return(false);
	}

	public int getQueryTimeout() throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Retrieves the current result as a ResultSet object. This method should
	 * be called only once per result.
	 *
	 * @return the current result as a ResultSet object or null if the result
	 *         is an update count or there are no more results
	 * @throws SQLException if a database access error occurs
	 */
	public ResultSet getResultSet() throws SQLException{
		if (header == null || header.getQueryType() != Q_TABLE) return(null);

		try {
			return(new MonetResultSet(
						this,
						header));
		} catch (IllegalArgumentException e) {
			throw new SQLException(e.toString());
		}
		// don't catch SQLException, it is declared to be thrown
	}

	/**
	 * Retrieves the result set concurrency for ResultSet objects generated
	 * by this Statement object.
	 *
	 * @return either ResultSet.CONCUR_READ_ONLY or ResultSet.CONCUR_UPDATABLE
	 */
	public int getResultSetConcurrency() {
		return(resultSetConcurrency);
	}

	public int getResultSetHoldability() throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Retrieves the result set type for ResultSet objects generated by this
	 * Statement object.
	 *
	 * @return one of ResultSet.TYPE_FORWARD_ONLY,
	 *         ResultSet.TYPE_SCROLL_INSENSITIVE, or
	 *         ResultSet.TYPE_SCROLL_SENSITIVE
	 */
	public int getResultSetType() {
		return(resultSetType);
	}

	/**
	 * Retrieves the current result as an update count; if the result is a
	 * ResultSet object or there are no more results, -1 is returned. This
	 * method should be called only once per result.
	 *
	 * @return the current result as an update count; -1 if the current result
	 *         is a ResultSet object or there are no more results
	 * @throws SQLException if a database access error occurs
	 */
	public int getUpdateCount() throws SQLException {
		if (header == null) return(-1);

		int ret = -1;
		if (header.getQueryType() == Q_UPDATE) {
			String tmpLine = header.getLine(0);
			try {
				if (tmpLine == null) throw new NumberFormatException("");
				ret = Integer.parseInt(tmpLine.substring(1, tmpLine.length() - 1).trim());
			} catch (NumberFormatException e) {
				throw new SQLException("Server sent unparsable update count: " + tmpLine);
			}
			// close the header, we got all it's information by now
			header.close();
		} else if (header.getQueryType() == Q_SCHEMA) {
			// this is an create, drop, alter
			// send special value for success with no additional info
			ret = SUCCESS_NO_INFO;
		}

		return(ret);
	}

	/**
	 * Retrieves the first warning reported by calls on this Statement object.
	 * If there is more than one warning, subsequent warnings will be chained to
	 * the first one and can be retrieved by calling the method
	 * SQLWarning.getNextWarning on the warning that was retrieved previously.
	 * <br /><br />
	 * This method may not be called on a closed statement; doing so will cause
	 * an SQLException to be thrown.
	 * <br /><br />
	 * Note: Subsequent warnings will be chained to this SQLWarning.
	 *
	 * @return the first SQLWarning object or null if there are none
	 * @throws SQLException if a database access error occurs or this method is
	 *         called on a closed connection
	 */
	public SQLWarning getWarnings() throws SQLException {
		if (closed) throw new SQLException("Cannot call on closed Statement");

		// if there are no warnings, this will be null, which fits with the
		// specification.
		return(warnings);
	}

	public void setCursorName(String name) throws SQLException { throw new SQLException("Method not supported, sorry!"); }
	public void setEscapeProcessing(boolean enable) throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Gives the driver a hint as to the direction in which rows will be
	 * processed in ResultSet objects created using this Statement object.
	 * The default value is ResultSet.FETCH_FORWARD.
	 * <br /><br />
	 * Note that this method sets the default fetch direction for result sets
	 * generated by this Statement object. Each result set has its own methods
	 * for getting and setting its own fetch direction.
	 *
	 * @param direction the initial direction for processing rows
	 * @throws SQLException if a database access error occurs or the given
	 *         direction is not one of ResultSet.FETCH_FORWARD,
	 *         ResultSet.FETCH_REVERSE, or ResultSet.FETCH_UNKNOWN
	 */
	public void setFetchDirection(int direction) throws SQLException {
		if (direction == ResultSet.FETCH_FORWARD ||
		    direction == ResultSet.FETCH_REVERSE ||
			direction == ResultSet.FETCH_UNKNOWN)
		{
			fetchDirection = direction;
		} else {
			throw new SQLException("Illegal direction: " + direction);
		}
	}

	/**
	 * Gives the JDBC driver a hint as to the number of rows that should be
	 * fetched from the database when more rows are needed. The number of rows
	 * specified affects only result sets created using this statement. If the
	 * value specified is zero, then the hint is ignored.
	 *
	 * @param rows the number of rows to fetch
	 * @throws SQLException if the condition 0 <= rows <= this.getMaxRows()
	 *         is not satisfied.
	 */
	public void setFetchSize(int rows) throws SQLException {
		if (rows >= 0 && !(getMaxRows() != 0 && rows > getMaxRows())) {
			fetchSize = rows;
		} else {
			throw new SQLException("Illegal fetch size value: " + rows);
		}
	}

	public void setMaxFieldSize(int max) throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	/**
	 * Sets the limit for the maximum number of rows that any ResultSet object
	 * can contain to the given number. If the limit is exceeded, the excess
	 * rows are silently dropped.
	 *
	 * @param max the new max rows limit; zero means there is no limit
	 * @throws SQLException if the condition max >= 0 is not satisfied
	 */
	public void setMaxRows(int max) throws SQLException {
		if (max < 0) throw new SQLException("Illegal max value: " + max);
		maxRows = max;
	}

	public void setQueryTimeout(int seconds) throws SQLException { throw new SQLException("Method not supported, sorry!"); }

	//== end methods of interface Statement

	protected void finalize() throws Throwable {
		close();
		super.finalize();
	}

	/**
	 * Adds a warning to the pile of warnings this Statement object has. If
	 * there were no warnings (or clearWarnings was called) this warning will
	 * be the first, otherwise this warning will get appended to the current
	 * warning.
	 *
	 * @param reason the warning message
	 */
	private void addWarning(String reason) {
		if (warnings == null) {
			warnings = new SQLWarning(reason);
		} else {
			warnings.setNextWarning(new SQLWarning(reason));
		}
	}
}
