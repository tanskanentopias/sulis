*** Settings ***
Library   String
Library   SerialLibrary

*** Variables ***
${com}   					/dev/ttyACM0
${baud} 					115200
${board}					nRF
${seq}      				RYGX
${correctTimeSeq}			000230X
${incorrectTimeSeq}			000074X
${incorrectSeqLenUnder}		00001X
${incorrectSeqLenOver}		0011220X
${zeroSeq}					000000X
${notDigit}					AAAAAAX
${correctReturnedTime}		150X
${incorrectReturnedTime}	74X
${errorValue}    			-1X
${errorLen}					-2X
${errorDigit}				-3X

*** Test Cases ***
Connect Serial
	Log To Console  Connecting to ${board}
	Add Port  ${com}  baudrate=${baud}  encoding=ascii
	Port Should Be Open  ${com}
	Reset Input Buffer
	Reset Output Buffer

Correct Time sequence
	Write Data   ${correctTimeSeq}   encoding=ascii 
	Log To Console   Send sequence ${correctTimeSeq}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${correctReturnedTime}
	Log To Console   Tested ${read} is same as ${correctReturnedTime}

Incorrect Time Sequence
	Write Data   ${incorrectTimeSeq}   encoding=ascii 
	Log To Console   Send sequence ${incorrectTimeSeq}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${errorValue}
	Log To Console   Tested ${read} is same as ${errorValue}

Time Sequence Cant Be Zero
	Write Data   ${zeroSeq}   encoding=ascii 
	Log To Console   Send sequence ${zeroSeq}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${errorValue}
	Log To Console   Tested ${read} is same as ${errorValue}

Time Sequence Incorrect Length Under
	Write Data   ${incorrectSeqLenUnder}   encoding=ascii 
	Log To Console   Send sequence ${incorrectSeqLenUnder}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${errorLen}
	Log To Console   Tested ${read} is same as ${errorLen}

Time Sequence Incorrect Length Over
	Write Data   ${incorrectSeqLenOver}   encoding=ascii 
	Log To Console   Send sequence ${incorrectSeqLenOver}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${errorLen}
	Log To Console   Tested ${read} is same as ${errorLen}

Time Sequence Not Digit
	Write Data   ${notDigit}   encoding=ascii 
	Log To Console   Send sequence ${notDigit}

	# vastaanotetaan merkkijono kunnes lopetusmerkki X (58) 
	${read} =   Read Until   terminator=58   encoding=ascii 

	# konsolille näkyviin vastaanotettu merkkijono
	Log To Console   Received ${read}
	
	# vertaillaan merkkijonoa
	Should Be Equal As Strings   ${read}    ${errorDigit}
	Log To Console   Tested ${read} is same as ${errorDigit}

Disconnect Serial
	Log To Console  Disconnecting ${board}
	[TearDown]  Delete Port  ${com}


	
	
	

