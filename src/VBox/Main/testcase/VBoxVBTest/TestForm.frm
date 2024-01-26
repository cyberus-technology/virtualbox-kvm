VERSION 5.00
Begin VB.Form TestForm 
   Caption         =   "VirtualBox Test"
   ClientHeight    =   4692
   ClientLeft      =   60
   ClientTop       =   348
   ClientWidth     =   6972
   LinkTopic       =   "TestForm"
   ScaleHeight     =   4692
   ScaleWidth      =   6972
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox machineList 
      Height          =   2352
      ItemData        =   "TestForm.frx":0000
      Left            =   240
      List            =   "TestForm.frx":0007
      TabIndex        =   4
      Top             =   2040
      Width           =   6492
   End
   Begin VB.CommandButton getMachieListCmd 
      Caption         =   "Get Machine List"
      Height          =   372
      Left            =   2640
      TabIndex        =   0
      Top             =   720
      Width           =   1692
   End
   Begin VB.Label Label3 
      AutoSize        =   -1  'True
      Caption         =   "Registered Machines:"
      Height          =   192
      Left            =   240
      TabIndex        =   5
      Top             =   1680
      Width           =   1572
   End
   Begin VB.Label versionLabel 
      AutoSize        =   -1  'True
      Caption         =   "<none>"
      Height          =   192
      Left            =   1680
      TabIndex        =   3
      Top             =   1320
      Width           =   528
   End
   Begin VB.Label Label2 
      AutoSize        =   -1  'True
      Caption         =   "VirtualBox Version:"
      Height          =   252
      Left            =   240
      TabIndex        =   2
      Top             =   1320
      Width           =   1344
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   $"TestForm.frx":0013
      Height          =   372
      Left            =   240
      TabIndex        =   1
      Top             =   120
      Width           =   6492
      WordWrap        =   -1  'True
   End
End
Attribute VB_Name = "TestForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Declare Function SetEnvironmentVariable Lib "kernel32" _
    Alias "SetEnvironmentVariableA" (ByVal lpName As String, ByVal lpValue As String) As Long
Private Declare Function GetEnvironmentVariable Lib "kernel32" _
    Alias "GetEnvironmentVariableA" (ByVal lpName As String, ByVal lpValue As String, ByVal nSize As Long) As Long

Private Sub Form_Load()
    
    ' Set where to take VirtualBox configuration from
    
    'SetEnvironmentVariable "VBOX_USER_HOME", "E:\VirtualBoxHome\win"
    
    ' Setup debug logging (available only in debug builds)
    
    'PATH_OUT_BASE = "D:/Coding/innotek/vbox/out"
    
    'SetEnvironmentVariable "VBOX_LOG", "main.e.l.f + gui.e.l.f"
    'SetEnvironmentVariable "VBOX_LOG_FLAGS", "time tid thread"
    'SetEnvironmentVariable "VBOX_LOG_DEST", "dir:" + PATH_OUT_BASE + "/logs"

End Sub

Private Sub getMachieListCmd_Click()
    
    ' Clear the old list contents
    
    machineList.Clear
    machineList.Refresh
    
    versionLabel.Caption = "<none>"
    
    ' Disable the button and the list for the duration of the call
        
    getMachieListCmd.Enabled = False
    machineList.Enabled = False
        
    ' Obtain the global VirtualBox object (this will start
    ' the VirtualBox server if it is not already started)
    
    Dim vbox As VirtualBox.VirtualBox
    Set vbox = New VirtualBox.VirtualBox
    
    ' Get the VirtualBox server version
    
    versionLabel.Caption = vbox.Version
    
    ' Obtain a list of registered machines
    
    Dim machines() As VirtualBox.IMachine
    machines = vbox.Machines2
    
    If UBound(machines) < 0 Then
        machineList.AddItem ("<none>")
    Else
        For i = 0 To UBound(machines)
            Item = machines(i).Name + " (" + machines(i).OSTypeId() + ")"
            machineList.AddItem (Item)
        Next i
    End If
    
    ' Reenable the button and the list
    
    getMachieListCmd.Enabled = True
    machineList.Enabled = True

End Sub
