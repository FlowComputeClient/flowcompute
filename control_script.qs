function Controller() {
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
	
	// Set the executable to launch and the checkbox label
    installer.setValue("RunProgram", "@TargetDir@/bin/FlowCompute.exe");
    installer.setValue("RunProgramDescription", "Launch FlowCompute");	
}

Controller.prototype.IntroductionPageCallback = function() {
    var page = gui.pageWidgetByObjectName("IntroductionPage");

    if (page) {
        page.MessageLabel.setText(
            "<p>Welcome to <b>FlowCompute</b>.</p>"
            + "<p>An open-source graphical client for OpenFOAM.</p>"
            + "<br>"
            + "<p>Click <b>Next</b> to continue.</p>"
        );
    }
};

Controller.prototype.FinishedPageCallback = function() {
    var checkBox = gui.currentPageWidget().RunItCheckBox;
    if (checkBox) {
        checkBox.checked = true;
    }
};