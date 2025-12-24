add_executable(commandline
    main.c
    ../../client/args.c
    ../../client/args.h
)

set_property(TARGET commandline PROPERTY C_STANDARD 23)
endtest(commandline)