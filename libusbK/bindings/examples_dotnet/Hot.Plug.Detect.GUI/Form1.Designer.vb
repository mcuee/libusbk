Imports System
Namespace Hot.Plug.Detect.GUI
    Partial Class Form1
        ''' <summary>
        ''' Required designer variable.
        ''' </summary>
        Private components As ComponentModel.IContainer = Nothing

        ''' <summary>
        ''' Clean up any resources being used.
        ''' </summary>
        ''' <paramname="disposing">true if managed resources should be disposed; otherwise, false.</param>
        Protected Overrides Sub Dispose(ByVal disposing As Boolean)
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If

            MyBase.Dispose(disposing)
        End Sub

#Region "Windows Form Designer generated code"

        ''' <summary>
        ''' Required method for Designer support - do not modify
        ''' the contents of this method with the code editor.
        ''' </summary>
        Private Sub InitializeComponent()
            dgvDevices = New Windows.Forms.DataGridView()
            label1 = New Windows.Forms.Label()
            colSymbolicLink = New Windows.Forms.DataGridViewTextBoxColumn()
            colDeviceDesc = New Windows.Forms.DataGridViewTextBoxColumn()
            colInstanceID = New Windows.Forms.DataGridViewTextBoxColumn()
            CType(dgvDevices, ComponentModel.ISupportInitialize).BeginInit()
            SuspendLayout()
            ' 
            ' dgvDevices
            ' 
            dgvDevices.AllowUserToAddRows = False
            dgvDevices.AllowUserToDeleteRows = False
            dgvDevices.Anchor = Windows.Forms.AnchorStyles.Top Or Windows.Forms.AnchorStyles.Bottom Or Windows.Forms.AnchorStyles.Left Or Windows.Forms.AnchorStyles.Right
            dgvDevices.ColumnHeadersHeightSizeMode = Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize
            dgvDevices.Columns.AddRange(New Windows.Forms.DataGridViewColumn() {colSymbolicLink, colDeviceDesc, colInstanceID})
            dgvDevices.Location = New Drawing.Point(12, 38)
            dgvDevices.MultiSelect = False
            dgvDevices.Name = "dgvDevices"
            dgvDevices.ReadOnly = True
            dgvDevices.RowHeadersVisible = False
            dgvDevices.SelectionMode = Windows.Forms.DataGridViewSelectionMode.FullRowSelect
            dgvDevices.ShowEditingIcon = False
            dgvDevices.Size = New Drawing.Size(419, 179)
            dgvDevices.TabIndex = 0
            ' 
            ' label1
            ' 
            label1.AutoSize = True
            label1.Location = New Drawing.Point(12, 22)
            label1.Name = "label1"
            label1.Size = New Drawing.Size(104, 13)
            label1.TabIndex = 1
            label1.Text = "Connected Devices:"
            ' 
            ' colSymbolicLink
            ' 
            colSymbolicLink.HeaderText = "SymbolicLink"
            colSymbolicLink.Name = "colSymbolicLink"
            colSymbolicLink.ReadOnly = True
            colSymbolicLink.Visible = False
            ' 
            ' colDeviceDesc
            ' 
            colDeviceDesc.AutoSizeMode = Windows.Forms.DataGridViewAutoSizeColumnMode.Fill
            colDeviceDesc.HeaderText = "Name"
            colDeviceDesc.Name = "colDeviceDesc"
            colDeviceDesc.ReadOnly = True
            ' 
            ' colInstanceID
            ' 
            colInstanceID.AutoSizeMode = Windows.Forms.DataGridViewAutoSizeColumnMode.Fill
            colInstanceID.HeaderText = "Instance ID"
            colInstanceID.Name = "colInstanceID"
            colInstanceID.ReadOnly = True
            ' 
            ' Form1
            ' 
            AutoScaleDimensions = New Drawing.SizeF(6F, 13F)
            AutoScaleMode = Windows.Forms.AutoScaleMode.Font
            ClientSize = New Drawing.Size(443, 229)
            Controls.Add(label1)
            Controls.Add(dgvDevices)
            Name = "Form1"
            Text = "Hot Plug Detection GUI Example"
            AddHandler Load, New EventHandler(AddressOf Form1_Load)
            AddHandler FormClosing, New Windows.Forms.FormClosingEventHandler(AddressOf Form1_FormClosing)
            CType(dgvDevices, ComponentModel.ISupportInitialize).EndInit()
            ResumeLayout(False)
            PerformLayout()
        End Sub

#End Region

        Private dgvDevices As Windows.Forms.DataGridView
        Private label1 As Windows.Forms.Label
        Private colSymbolicLink As Windows.Forms.DataGridViewTextBoxColumn
        Private colDeviceDesc As Windows.Forms.DataGridViewTextBoxColumn
        Private colInstanceID As Windows.Forms.DataGridViewTextBoxColumn
    End Class
End Namespace
