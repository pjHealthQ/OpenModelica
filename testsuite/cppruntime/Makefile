TEST = ../rtest -v

TESTFILES = \
WhenStatement1.mos \
BouncingBall.mos \




# Dependency files that are not .mo .mos or Makefile
# Add them here or they will be cleaned.
DEPENDENCIES = \
*.mo \
*.mos \
FlatParse.mof \
Makefile \
EventTests.mat \
ExtObj.c \
ExtObj.h \
ExtObjStringParam.ext.c \
ExtObjStringParam.ext.h \
ExternalFunc1.c \
ExternalFunc1_ext.h \
ExternalFunc2.c \
ExternalFunc2.h \
Func.c \
Func.h \
InOutStrings_fkn1.cc \
InOutStrings_fkn2.cc \
ModelicaUtilities.myExtFunction.c \
dependencies.dep \
testTables.txt \
testTables2.csv \
testTables2.txt \
testTables2D.txt \


CLEAN = `ls | grep -w -v -f deps.tmp`

.PHONY : test clean getdeps

test:
	@echo
	@echo Running tests...
	@echo
	@echo OPENMODELICAHOME=" $(OPENMODELICAHOME) "
	@$(TEST) $(TESTFILES)
	
# Cleans all files that are not listed as dependencies 
clean :
	@echo $(DEPENDENCIES) | sed 's/ /\\|/g' > deps.tmp
	@rm -f $(CLEAN)

# Run this if you want to list out the files (dependencies).
# do it after cleaning and updating the folder
# then you can get a list of file names (which must be dependencies
# since you got them from repository + your own new files)
# then add them to the DEPENDENCIES. You can find the 
# list in deps.txt 
getdeps: 
	@echo $(DEPENDENCIES) | sed 's/ /\\|/g' > deps.tmp
	@echo $(CLEAN) | sed -r 's/deps.txt|deps.tmp//g' | sed 's/ / \\\n/g' > deps.txt	
	@echo Dependency list saved in deps.txt.
	@echo Copy the list from deps.txt and add it to the Makefile @DEPENDENCIES

failingtest :
	@echo
	@echo Running failing tests...
	@echo
	@$(TEST) $(FAILINGTESTFILES)
