pgSphere
========

Fork from [http://pgfoundry.org/projects/pgsphere/](http://pgfoundry.org/projects/pgsphere/), document: [http://pgsphere.projects.pgfoundry.org/](http://pgsphere.projects.pgfoundry.org/)



### Install

First unpack the pgSphere sources:
	shell> tar -xzf path_to_pg_sphere_xxx.tgz
            
Now, change into the pg_sphere directory and run:

	shell> make USE_PGXS=1 PG_CONFIG=/path/to/pg_config
            
To install pgSphere you have to run :

	shell> make USE_PGXS=1 PG_CONFIG=/path/to/pg_config install
            
To check the installation change into the pg_sphere source directory again and run:

	shell> make installcheck

### Create database with pgSphere

We assume you have already created a database datab, where datab is the name of any database. Now change into the directory `POSTGRESQL_INSTALL_PATH/share/contrib`. Presupposing the name of your PostgreSQL's superuser is `postgres` type:

	shell> psql -U postgres datab < pg_sphere.sql
            