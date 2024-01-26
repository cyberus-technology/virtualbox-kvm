; StrStr
 ; input, top of stack = string to search for
 ;        top of stack-1 = string to search in
 ; output, top of stack (replaces with the portion of the string remaining)
 ; modifies no other variables.
 ;
 ; Usage:
 ;   Push "this is a long ass string"
 ;   Push "ass"
 ;   Call StrStr
 ;   Pop $R0
 ;  ($R0 at this point is "ass string")


!macro StrStr un
 Function ${un}StrStr
   Exch $R1 ; st=haystack,old$R1, $R1=needle
   Exch    ; st=old$R1,haystack
   Exch $R2 ; st=old$R1,old$R2, $R2=haystack
   Push $R3
   Push $R4
   Push $R5
   StrLen $R3 $R1
   StrCpy $R4 0
   ; $R1=needle
   ; $R2=haystack
   ; $R3=len(needle)
   ; $R4=cnt
   ; $R5=tmp
   loop:
     StrCpy $R5 $R2 $R3 $R4
     StrCmp $R5 $R1 done
     StrCmp $R5 "" done
     IntOp $R4 $R4 + 1
     Goto loop
 done:
   StrCpy $R1 $R2 "" $R4
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Exch $R1
 FunctionEnd
!macroend
!ifndef UNINSTALLER_ONLY
  !insertmacro StrStr ""
!endif
!insertmacro StrStr "un."

!macro StrStrAdv un
Function ${un}StrStrAdv

  ; After this point:
  ; $0 = String (input)
  ; $1 = SubString (input)
  ; $2 = SearchDirection (input)
  ; $3 = StrInclusionDirection (input)
  ; $4 = IncludeSubString (input)
  ; $5 = Loops (input)
  ; $6 = CaseSensitive (input)
  ; $7 = StringLength (temp)
  ; $8 = StrToSearchLength (temp)
  ; $9 = CurrentLoop (temp)
  ; $R0 = EndCharPos (temp)
  ; $R1 = StartCharPos (temp)
  ; $R2 = ResultVar (output)
  ; $R3 = Temp (temp)

  ; Get input from user

  Exch $6
  Exch
  Exch $5
  Exch
  Exch 2
  Exch $4
  Exch 2
  Exch 3
  Exch $3
  Exch 3
  Exch 4
  Exch $2
  Exch 4
  Exch 5
  Exch $1
  Exch 5
  Exch 6
  Exch $0
  Exch 6
  Push $7
  Push $8
  Push $9
  Push $R3
  Push $R2
  Push $R1
  Push $R0

  ; Clean $R0-$R3 variables
  StrCpy $R0 ""
  StrCpy $R1 ""
  StrCpy $R2 ""
  StrCpy $R3 ""

  ; Verify if we have the correct values on the variables
  ${If} $0 == ``
    SetErrors ;AdvStrStr_StrToSearch not found
    Goto AdvStrStr_End
  ${EndIf}

  ${If} $1 == ``
    SetErrors ;No text to search
    Goto AdvStrStr_End
  ${EndIf}

  ${If} $2 != <
    StrCpy $2 >
  ${EndIf}

  ${If} $3 != <
    StrCpy $3 >
  ${EndIf}

  ${If} $4 <> 0
    StrCpy $4 1
  ${EndIf}

  ${If} $5 <= 0
    StrCpy $5 0
  ${EndIf}

  ${If} $6 <> 1
    StrCpy $6 0
  ${EndIf}

  ; Find "AdvStrStr_String" length
  StrLen $7 $0

  ; Then find "AdvStrStr_SubString" length
  StrLen $8 $1

  ; Now set up basic variables

  ${If} $2 == <
    IntOp $R1 $7 - $8
    StrCpy $R2 $7
  ${Else}
    StrCpy $R1 0
    StrCpy $R2 $8
  ${EndIf}

  StrCpy $9 0 ; First loop

  ;Let's begin the search

  ${Do}
    ; Step 1: If the starting or ending numbers are negative
    ;         or more than AdvStrStr_StringLen, we return
    ;         error

    ${If} $R1 < 0
      StrCpy $R1 ``
      StrCpy $R2 ``
      StrCpy $R3 ``
      SetErrors ;AdvStrStr_SubString not found
      Goto AdvStrStr_End
    ${ElseIf} $R2 > $7
      StrCpy $R1 ``
      StrCpy $R2 ``
      StrCpy $R3 ``
      SetErrors ;AdvStrStr_SubString not found
      Goto AdvStrStr_End
    ${EndIf}

    ; Step 2: Start the search depending on
    ;         AdvStrStr_SearchDirection. Chop down not needed
    ;         characters.

    ${If} $R1 <> 0
      StrCpy $R3 $0 `` $R1
    ${EndIf}

    ${If} $R2 <> $7
      ${If} $R1 = 0
        StrCpy $R3 $0 $8
      ${Else}
        StrCpy $R3 $R3 $8
      ${EndIf}
    ${EndIf}

    ; Step 3: Make sure that's the string we want

    ; Case-Sensitive Support <- Use "AdvStrStr_Temp"
    ; variable because it won't be used anymore

    ${If} $6 == 1
      System::Call `kernel32::lstrcmpA(ts, ts) i.s` `$R3` `$1`
      Pop $R3
      ${If} $R3 = 0
        StrCpy $R3 1 ; Continue
      ${Else}
        StrCpy $R3 0 ; Break
      ${EndIf}
    ${Else}
      ${If} $R3 == $1
        StrCpy $R3 1 ; Continue
      ${Else}
        StrCpy $R3 0 ; Break
      ${EndIf}
    ${EndIf}

    ; After the comparison, confirm that it is the
    ; value we want.

    ${If} $R3 = 1

      ;We found it, return except if the user has set up to
      ;search for another one:
      ${If} $9 >= $5

        ;Now, let's see if the user wants
        ;AdvStrStr_SubString to appear:
        ${If} $4 == 0
          ;Return depends on AdvStrStr_StrInclusionDirection
          ${If} $3 == <
            ; RTL
            StrCpy $R0 $0 $R1
          ${Else}
            ; LTR
            StrCpy $R0 $0 `` $R2
          ${EndIf}
          ${Break}
        ${Else}
          ;Return depends on AdvStrStr_StrInclusionDirection
          ${If} $3 == <
            ; RTL
            StrCpy $R0 $0 $R2
          ${Else}
            ; LTR
            StrCpy $R0 $0 `` $R1
          ${EndIf}
          ${Break}
        ${EndIf}
      ${Else}
        ;If the user wants to have more loops, let's do it so!
        IntOp $9 $9 + 1

        ${If} $2 == <
          IntOp $R1 $R1 - 1
          IntOp $R2 $R2 - 1
        ${Else}
          IntOp $R1 $R1 + 1
          IntOp $R2 $R2 + 1
        ${EndIf}
      ${EndIf}
    ${Else}
      ; Step 4: We didn't find it, so do steps 1 thru 3 again

      ${If} $2 == <
        IntOp $R1 $R1 - 1
        IntOp $R2 $R2 - 1
      ${Else}
        IntOp $R1 $R1 + 1
        IntOp $R2 $R2 + 1
      ${EndIf}
    ${EndIf}
  ${Loop}

  AdvStrStr_End:

  ;Add 1 to AdvStrStr_EndCharPos to be supportable
  ;by "StrCpy"

  IntOp $R2 $R2 - 1

  ;Return output to user

  Exch $R0
  Exch
  Pop $R1
  Exch
  Pop $R2
  Exch
  Pop $R3
  Exch
  Pop $9
  Exch
  Pop $8
  Exch
  Pop $7
  Exch
  Pop $6
  Exch
  Pop $5
  Exch
  Pop $4
  Exch
  Pop $3
  Exch
  Pop $2
  Exch
  Pop $1
  Exch
  Pop $0

FunctionEnd
!macroend
!insertmacro StrStrAdv ""
;!insertmacro StrStrAdv "un."
