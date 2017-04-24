#!/bin/bash

CLASSPATH=./Sux4J/sux4j-4.0.1/sux4j-4.0.1.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-collections.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-configuration.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-io.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-lang.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-logging.jar:./Sux4J/sux4j-4.0.1/jars/runtime/commons-math3.jar:./Sux4J/sux4j-4.0.1/jars/runtime/dsiutils.jar:./Sux4J/sux4j-4.0.1/jars/runtime/fastutil.jar:./Sux4J/sux4j-4.0.1/jars/runtime/guava.jar:./Sux4J/sux4j-4.0.1/jars/runtime/jsap.jar:./Sux4J/sux4j-4.0.1/jars/runtime/logback-classic.jar:./Sux4J/sux4j-4.0.1/jars/runtime/logback-core.jar:./Sux4J/sux4j-4.0.1/jars/runtime/slf4j-api.jar:./Sux4J/slow/:./Sux4J/sux4j-4.0.1/jars/test/junit.jar:./Sux4J/sux4j-4.0.1/jars/test/hamcrest-core.jar

if [ ! -f ./Sux4J/sux4j-4.0.1/sux4j-4.0.1.jar ]; then
    bash -c "cd Sux4J && make binary"
fi
javac -cp $CLASSPATH Sux4J/slow/it/unimi/dsi/sux4j/mph/*.java
java -Djava.io.tmpdir=/tmp/ -cp $CLASSPATH org.junit.runner.JUnitCore it.unimi.dsi.sux4j.mph.GOV3FunctionSlowTest
