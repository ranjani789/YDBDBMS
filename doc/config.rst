
====================
Configuration
====================

.. contents::
   :depth: 5

Configuration settings can be passed to the program with the following precedence:

1. Flags passed to the program
2. Environment settings
3. octo.conf
4. ~/octo.conf
5. $ydb_dist/plugin/etc/octo.conf

--------------------
Config files
--------------------

Octo currently looks for a configuration file in the following directories:

* $ydb_dist/plugin/etc/octo.conf
* ~/octo.conf
* ./octo.conf

If the same setting exists in more than one configuration file the setting in the later file (according to the list above) will prevail. An example config file can be found in :code:`$ydb_dist/plugin/etc/octo.conf`.

Sample config file:

.. literalinclude:: ../src/aux/octo.conf.default

~~~~~~~~~~~~~~
Routines
~~~~~~~~~~~~~~

Octo requires that :code:`$ydb_dist/plugin/o/_ydbocto.so` and the path configured for :code:`routine_cache` in :code:`octo.conf` be part of :code:`$ydb_routines` - both for running the :code:`octo` and :code:`rocto` excutables and added to your normal environment setup scripts as YottaDB triggers are used to maintain cross references for Octo.

~~~~~~~~~~~~~
Logging
~~~~~~~~~~~~~

A config file can include instructions specifying verbosity for logging:

* TRACE: A TRACE message is useful to help identify issues in the code
* INFO: An INFO message is used to relay information to the user
* DEBUG: A DEBUG message helps users debug configuration issues
* WARNING: A WARNING message warns the user about potential problems in the code
* ERROR : An ERROR message informs the user that an error has occurred
* FATAL: A FATAL message terminates the program

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Locations and Global Variables
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Octo supports two different ways of mapping its globals to a YottaDB database: Using normal global mapping procedures for an existing global directory OR using a separate global directory. All octo globals are prefixed with :code:`^%ydbocto`.

Other important configuration information required for the database that holds Octo globals:

* :code:`NULL_SUBSCRIPTS` must be set to :code:`ALWAYS`.

  Example: :code:`$ydb_dist/mupip set -null_subscripts=true -region 'OCTO'`

* :code:`KEY_SIZE` must be tuned to your data - this can be set to the maximum allowed by YottaDB - "1019"

   Example: :code:`$ydb_dist/mupip set -key_size=1019 -region 'OCTO'`

* :code:`RECORD_SIZE` must be tuned to your data/queries - a reasonable starting value is "300000"

  Example: :code:`$ydb_dist/mupip set -record_size=300000 -region 'OCTO'`

+++++++++++++++++++++++++++++++++++
Using an existing Global Directory
+++++++++++++++++++++++++++++++++++

The :code:`octo_global_prefix` can be set to a value that will then be prefixed to the M global variables used in Octo.

Octo prefixes all of its globals with :code:`%ydbocto`. You can map :code:`%ydbocto*` to a separate database (highly recommended) that meets the above requirements.

Example:

.. parsed-literal::
   octo_global_prefix = "%ydbocto"

The global variable :code:`^schema` will be :code:`^%ydboctoschema` as a global variable in Octo.

+++++++++++++++++++++++++++++++++++
Using a separate global directory
+++++++++++++++++++++++++++++++++++

To use a separate global directory with Octo, you must change the :code:`octo_global_directory` configuration in :code:`octo.conf` to point to the path that contains the global directory. Using a full path to the global directory is recommended.

For example:

.. parsed-literal::
   octo_global_directory = "mumps.gld"

All globals should be preceded by :code:`^|<octo_global_directory>|<octo_global_prefix>`

Example:

.. parsed-literal::
   ^|mumps.gld|%ydboctoocto

Some of the globals used in Octo are:

* **octo**: This global can refer to various functions, variables, octo "read only" table values (postgres mappings, oneRowTable, etc.), and some counts. It needs to be journaled and persist between sessions.

* **session**: This global can contain session variables, portals and prepared queries. It need not be journaled/ persist between sessions since it only contains data related to the current session.

* **cursor**: This global contains output data and temporary tables. It need not be journaled/ persist between sessions since it only contains temporary data.

* **xref**: This global contains cross-references, row counts and other statistical information. It needs to be journaled and persist between sessions.

* **schema**: This global contains information about the tables loaded into the database. It needs to be journaled and persist between sessions.

+++++++++++++++++++++++
TLS/SSL Configuration
+++++++++++++++++++++++

Enabling TLS/SSL requires several additional steps beyond installing the YottaDB encryption plugin - it requires creating a Certificate Authority (CA), generating a TLS/SSL certificate, and making additional changes to :code:`octo.conf`.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Generate CA key and certificate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. parsed-literal::

   \# In a directory in which you want to store all of the certificates for Octo
   \# Be sure to create a good passphrase for the CA
   openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out CA.key
   \# This creates a CA valid for 1 Year and interactively prompts for additional information
   openssl req -new -nodes -key CA.key -days 365 -x509 -out CA.crt

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Create server key and certificate request
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. parsed-literal::

   \# This creates a 2048 bit private key
   openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out server.key
   \# This creeates the certificate signing request
   openssl req -new -key server.key -out server.csr

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Sign certificate based on request and local CA
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. parsed-literal::

   \# Asks the CA to sign the certificate with a 1 Year validity time
   openssl x509 -req -in server.csr -CA CA.crt -CAkey CA.key -CAcreateserial \ -out server.crt -days 365
   \# Mask the password for the certificate in a way YottaDB understands
   $ydb_dist/plugin/gtmcrypt/maskpass
   \# This will need to be added to any startup scripts for octo/rocto
   export ydb_tls_passwd_OCTOSERVER=[Masked Password from maskpass]
   export ydb_crypt_config=/path/to/octo.conf

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Update Octo configuration file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The $ydb_dist/plugin/etc/octo.conf contains an outline of the minimum configuration options needed to enable TLS/SSL. The key items are:

1. In the "rocto" section, "ssl_on" must be set to "true" (no quotes needed in the conf).
2. A "tls" section must be present and generally conform to the requirements specified for the `TLS plugin itself <https://docs.yottadb.com/AdminOpsGuide/tls.html>`_. Other notes:

      * Octo doesn't use any of the "dh*" settings, so those can be omitted.
      * The "format" specifier can also be omitted, as long as the certs are in PEM format.
      * The "CAfile" and "CApath" fields are mandatory and must point to valid files/locations with a full path.
      * A subsection named "OCTOSERVER" with "key", and "cert" settings specifying the names of the private key and cert files.

3. The :code:`ydb_tls_passwd_OCTOSERVER` and :code:`ydb_crypt_config` environment variables must be set correctly.

----------------------------
Usage
----------------------------

Before running Octo/Rocto make sure that the required YottaDB variables are set either by creating your own script or run :code:`source $ydb_dist/ydb_env_set`.

To use the command-line SQL interpreter run: :code:`$ydb_dist/plugin/bin/octo`.

To use the PostgreSQL protocol compatible server run: :code:`$ydb_dist/plugin/bin/rocto`.
