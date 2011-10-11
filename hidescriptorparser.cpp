/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
*/
#include "hidescriptorparser.h"
//#include "hidusagetitlearrays.cpp"

const char *ReportDescParserBase::usagePageTitles0[]	PROGMEM = 
{
	pstrUsagePageGenericDesktopControls	,
	pstrUsagePageSimulationControls		,
	pstrUsagePageVRControls				,
	pstrUsagePageSportControls			,
	pstrUsagePageGameControls			,
	pstrUsagePageGenericDeviceControls	,
	pstrUsagePageKeyboardKeypad			,
	pstrUsagePageLEDs					,
	pstrUsagePageButton					,
	pstrUsagePageOrdinal				,	
	pstrUsagePageTelephone				,
	pstrUsagePageConsumer				,
	pstrUsagePageDigitizer				,
	pstrUsagePagePID					,
	pstrUsagePageUnicode					
};

const char *ReportDescParserBase::usagePageTitles1[]	PROGMEM = 
{
	pstrUsagePageBarCodeScanner			,
	pstrUsagePageScale					,
	pstrUsagePageMSRDevices				,
	pstrUsagePagePointOfSale			,	
	pstrUsagePageCameraControl			,
	pstrUsagePageArcade					
};
const char *ReportDescParserBase::genDesktopTitles0[] PROGMEM =
{
	pstrUsagePointer					,
	pstrUsageMouse						,
	pstrUsageJoystick					,
	pstrUsageGamePad					,	
	pstrUsageKeyboard					,
	pstrUsageKeypad						,
	pstrUsageMultiAxisController		,	
	pstrUsageTabletPCSystemControls		

};
const char *ReportDescParserBase::genDesktopTitles1[] PROGMEM =
{
	pstrUsageX							,
	pstrUsageY							,
	pstrUsageZ							,
	pstrUsageRx							,
	pstrUsageRy							,
	pstrUsageRz							,
	pstrUsageSlider						,
	pstrUsageDial						,
	pstrUsageWheel						,
	pstrUsageHatSwitch					,
	pstrUsageCountedBuffer				,
	pstrUsageByteCount					,
	pstrUsageMotionWakeup				,
	pstrUsageStart						,
	pstrUsageSelect						,
	pstrUsagePageReserved				,
	pstrUsageVx							,
	pstrUsageVy							,
	pstrUsageVz							,
	pstrUsageVbrx						,
	pstrUsageVbry						,
	pstrUsageVbrz						,
	pstrUsageVno						,	
	pstrUsageFeatureNotification		,	
	pstrUsageResolutionMultiplier		
};
const char *ReportDescParserBase::genDesktopTitles2[] PROGMEM =
{
	pstrUsageSystemControl		,
	pstrUsageSystemPowerDown	,	
	pstrUsageSystemSleep		,	
	pstrUsageSystemWakeup		,
	pstrUsageSystemContextMenu	,
	pstrUsageSystemMainMenu		,
	pstrUsageSystemAppMenu		,
	pstrUsageSystemMenuHelp		,
	pstrUsageSystemMenuExit		,
	pstrUsageSystemMenuSelect	,
	pstrUsageSystemMenuRight	,	
	pstrUsageSystemMenuLeft		,
	pstrUsageSystemMenuUp		,
	pstrUsageSystemMenuDown		,
	pstrUsageSystemColdRestart	,
	pstrUsageSystemWarmRestart	,
	pstrUsageDPadUp				,
	pstrUsageDPadDown			,
	pstrUsageDPadRight			,
	pstrUsageDPadLeft			
};
const char *ReportDescParserBase::genDesktopTitles3[] PROGMEM =
{
	pstrUsageSystemDock				,
	pstrUsageSystemUndock			,
	pstrUsageSystemSetup			,	
	pstrUsageSystemBreak			,	
	pstrUsageSystemDebuggerBreak	,	
	pstrUsageApplicationBreak		,
	pstrUsageApplicationDebuggerBreak,
	pstrUsageSystemSpeakerMute		,
	pstrUsageSystemHibernate			
};
const char *ReportDescParserBase::genDesktopTitles4[] PROGMEM =
{
	pstrUsageSystemDisplayInvert		,	
	pstrUsageSystemDisplayInternal		,
	pstrUsageSystemDisplayExternal		,
	pstrUsageSystemDisplayBoth			,
	pstrUsageSystemDisplayDual			,
	pstrUsageSystemDisplayToggleIntExt	,
	pstrUsageSystemDisplaySwapPriSec	,
	pstrUsageSystemDisplayLCDAutoscale	
};
const char *ReportDescParserBase::simuTitles0[] PROGMEM = 
{
	pstrUsageFlightSimulationDevice		,
	pstrUsageAutomobileSimulationDevice	,
	pstrUsageTankSimulationDevice		,
	pstrUsageSpaceshipSimulationDevice	,
	pstrUsageSubmarineSimulationDevice	,
	pstrUsageSailingSimulationDevice	,	
	pstrUsageMotocicleSimulationDevice	,
	pstrUsageSportsSimulationDevice		,
	pstrUsageAirplaneSimulationDevice	,
	pstrUsageHelicopterSimulationDevice	,
	pstrUsageMagicCarpetSimulationDevice,
	pstrUsageBicycleSimulationDevice		
};
const char *ReportDescParserBase::simuTitles1[] PROGMEM = 
{
	pstrUsageFlightControlStick			,
	pstrUsageFlightStick				,	
	pstrUsageCyclicControl				,
	pstrUsageCyclicTrim					,
	pstrUsageFlightYoke					,
	pstrUsageTrackControl				
};
const char *ReportDescParserBase::simuTitles2[] PROGMEM = 
{
	pstrUsageAileron					,	
	pstrUsageAileronTrim				,	
	pstrUsageAntiTorqueControl			,
	pstrUsageAutopilotEnable			,	
	pstrUsageChaffRelease				,
	pstrUsageCollectiveControl			,
	pstrUsageDiveBrake					,
	pstrUsageElectronicCountermeasures	,
	pstrUsageElevator					,
	pstrUsageElevatorTrim				,
	pstrUsageRudder						,
	pstrUsageThrottle					,
	pstrUsageFlightCommunications		,
	pstrUsageFlareRelease				,
	pstrUsageLandingGear				,	
	pstrUsageToeBrake					,
	pstrUsageTrigger					,	
	pstrUsageWeaponsArm					,
	pstrUsageWeaponsSelect				,
	pstrUsageWingFlaps					,	
	pstrUsageAccelerator				,	
	pstrUsageBrake						,
	pstrUsageClutch						,
	pstrUsageShifter					,	
	pstrUsageSteering					,
	pstrUsageTurretDirection			,	
	pstrUsageBarrelElevation			,	
	pstrUsageDivePlane					,
	pstrUsageBallast					,	
	pstrUsageBicycleCrank				,
	pstrUsageHandleBars					,
	pstrUsageFrontBrake					,
	pstrUsageRearBrake					
};
const char *ReportDescParserBase::vrTitles0[]	PROGMEM = 
{
	pstrUsageBelt				,
	pstrUsageBodySuit			,
	pstrUsageFlexor				,
	pstrUsageGlove				,
	pstrUsageHeadTracker		,	
	pstrUsageHeadMountedDisplay	,
	pstrUsageHandTracker		,	
	pstrUsageOculometer			,
	pstrUsageVest				,
	pstrUsageAnimatronicDevice	
};
const char *ReportDescParserBase::vrTitles1[]	PROGMEM = 
{
	pstrUsageStereoEnable	,
	pstrUsageDisplayEnable	
};
const char *ReportDescParserBase::sportsCtrlTitles0[]	PROGMEM = 
{
	pstrUsageBaseballBat				,	
	pstrUsageGolfClub					,
	pstrUsageRowingMachine				,
	pstrUsageTreadmill					
};
const char *ReportDescParserBase::sportsCtrlTitles1[]	PROGMEM = 
{
	pstrUsageOar						,	
	pstrUsageSlope						,
	pstrUsageRate						,
	pstrUsageStickSpeed					,
	pstrUsageStickFaceAngle				,
	pstrUsageStickHeelToe				,
	pstrUsageStickFollowThough			,
	pstrUsageStickTempo					,
	pstrUsageStickType					,
	pstrUsageStickHeight					
};
const char *ReportDescParserBase::sportsCtrlTitles2[]	PROGMEM = 
{
	pstrUsagePutter						,
	pstrUsage1Iron						,
	pstrUsage2Iron						,
	pstrUsage3Iron						,
	pstrUsage4Iron						,
	pstrUsage5Iron						,
	pstrUsage6Iron						,
	pstrUsage7Iron						,
	pstrUsage8Iron						,
	pstrUsage9Iron						,
	pstrUsage10Iron						,
	pstrUsage11Iron						,
	pstrUsageSandWedge					,
	pstrUsageLoftWedge					,
	pstrUsagePowerWedge					,
	pstrUsage1Wood						,
	pstrUsage3Wood						,
	pstrUsage5Wood						,
	pstrUsage7Wood						,
	pstrUsage9Wood						
};
const char *ReportDescParserBase::gameTitles0[] PROGMEM =
{
	pstrUsage3DGameController		,
	pstrUsagePinballDevice			,
	pstrUsageGunDevice				
};
const char *ReportDescParserBase::gameTitles1[] PROGMEM =
{
	pstrUsagePointOfView			,	
	pstrUsageTurnRightLeft			,
	pstrUsagePitchForwardBackward	,
	pstrUsageRollRightLeft			,
	pstrUsageMoveRightLeft			,
	pstrUsageMoveForwardBackward	,	
	pstrUsageMoveUpDown				,
	pstrUsageLeanRightLeft			,
	pstrUsageLeanForwardBackward	,	
	pstrUsageHeightOfPOV			,	
	pstrUsageFlipper				,	
	pstrUsageSecondaryFlipper		,
	pstrUsageBump					,
	pstrUsageNewGame				,	
	pstrUsageShootBall				,
	pstrUsagePlayer					,
	pstrUsageGunBolt				,	
	pstrUsageGunClip				,	
	pstrUsageGunSelector			,	
	pstrUsageGunSingleShot			,
	pstrUsageGunBurst				,
	pstrUsageGunAutomatic			,
	pstrUsageGunSafety				,
	pstrUsageGamepadFireJump		,
	pstrUsageGamepadTrigger			
};
const char *ReportDescParserBase::genDevCtrlTitles[] PROGMEM = 
{
	pstrUsageBatteryStrength,
	pstrUsageWirelessChannel,			
	pstrUsageWirelessID,				
	pstrUsageDiscoverWirelessControl,	
	pstrUsageSecurityCodeCharEntered,	
	pstrUsageSecurityCodeCharErased,	
	pstrUsageSecurityCodeCleared		
};
const char *ReportDescParserBase::ledTitles[] PROGMEM =
{
	pstrUsageNumLock						,
	pstrUsageCapsLock					,
	pstrUsageScrollLock					,
	pstrUsageCompose					,	
	pstrUsageKana						,
	pstrUsagePower						,
	pstrUsageShift						,
	pstrUsageDoNotDisturb				,
	pstrUsageMute						,
	pstrUsageToneEnable					,
	pstrUsageHighCutFilter				,
	pstrUsageLowCutFilter				,
	pstrUsageEqualizerEnable			,	
	pstrUsageSoundFieldOn				,
	pstrUsageSurroundOn					,
	pstrUsageRepeat						,
	pstrUsageStereo						,
	pstrUsageSamplingRateDetect			,
	pstrUsageSpinning					,
	pstrUsageCAV						,	
	pstrUsageCLV						,	
	pstrUsageRecordingFormatDetect		,
	pstrUsageOffHook					,	
	pstrUsageRing						,
	pstrUsageMessageWaiting				,
	pstrUsageDataMode					,
	pstrUsageBatteryOperation			,
	pstrUsageBatteryOK					,
	pstrUsageBatteryLow					,
	pstrUsageSpeaker					,	
	pstrUsageHeadSet					,	
	pstrUsageHold						,
	pstrUsageMicrophone					,
	pstrUsageCoverage					,
	pstrUsageNightMode					,
	pstrUsageSendCalls					,
	pstrUsageCallPickup					,
	pstrUsageConference					,
	pstrUsageStandBy					,	
	pstrUsageCameraOn					,
	pstrUsageCameraOff					,
	pstrUsageOnLine						,
	pstrUsageOffLine					,	
	pstrUsageBusy						,
	pstrUsageReady						,
	pstrUsagePaperOut					,
	pstrUsagePaperJam					,
	pstrUsageRemote						,
	pstrUsageForward					,	
	pstrUsageReverse					,	
	pstrUsageStop						,
	pstrUsageRewind						,
	pstrUsageFastForward				,	
	pstrUsagePlay						,
	pstrUsagePause						,
	pstrUsageRecord						,
	pstrUsageError						,
	pstrUsageSelectedIndicator			,
	pstrUsageInUseIndicator				,
	pstrUsageMultiModeIndicator			,
	pstrUsageIndicatorOn				,	
	pstrUsageIndicatorFlash				,
	pstrUsageIndicatorSlowBlink			,
	pstrUsageIndicatorFastBlink			,
	pstrUsageIndicatorOff				,
	pstrUsageFlashOnTime				,	
	pstrUsageSlowBlinkOnTime			,	
	pstrUsageSlowBlinkOffTime			,
	pstrUsageFastBlinkOnTime			,	
	pstrUsageFastBlinkOffTime			,
	pstrUsageIndicatorColor				,
	pstrUsageIndicatorRed				,
	pstrUsageIndicatorGreen				,
	pstrUsageIndicatorAmber				,
	pstrUsageGenericIndicator			,
	pstrUsageSystemSuspend				,
	pstrUsageExternalPowerConnected		
};
const char *ReportDescParserBase::telTitles0			[] PROGMEM = 
{
	pstrUsagePhone				,
	pstrUsageAnsweringMachine	,
	pstrUsageMessageControls	,	
	pstrUsageHandset			,	
	pstrUsageHeadset			,	
	pstrUsageTelephonyKeyPad	,	
	pstrUsageProgrammableButton	
};
const char *ReportDescParserBase::telTitles1			[] PROGMEM = 
{
	pstrUsageHookSwitch					,
	pstrUsageFlash						,
	pstrUsageFeature					,	
	pstrUsageHold						,
	pstrUsageRedial						,
	pstrUsageTransfer					,
	pstrUsageDrop						,
	pstrUsagePark						,
	pstrUsageForwardCalls				,
	pstrUsageAlternateFunction			,
	pstrUsageLine						,
	pstrUsageSpeakerPhone				,
	pstrUsageConference				,
	pstrUsageRingEnable				,	
	pstrUsageRingSelect				,	
	pstrUsagePhoneMute				,	
	pstrUsageCallerID				,	
	pstrUsageSend						
};
const char *ReportDescParserBase::telTitles2			[] PROGMEM = 
{
	pstrUsageSpeedDial		,
	pstrUsageStoreNumber	,	
	pstrUsageRecallNumber	,
	pstrUsagePhoneDirectory
};
const char *ReportDescParserBase::telTitles3			[] PROGMEM = 
{
	pstrUsageVoiceMail		,
	pstrUsageScreenCalls	,	
	pstrUsageDoNotDisturb	,
	pstrUsageMessage		,	
	pstrUsageAnswerOnOff		
};
const char *ReportDescParserBase::telTitles4			[] PROGMEM = 
{
	pstrUsageInsideDialTone			,
	pstrUsageOutsideDialTone		,	
	pstrUsageInsideRingTone			,
	pstrUsageOutsideRingTone		,	
	pstrUsagePriorityRingTone		,
	pstrUsageInsideRingback			,
	pstrUsagePriorityRingback		,
	pstrUsageLineBusyTone			,
	pstrUsageReorderTone			,	
	pstrUsageCallWaitingTone		,	
	pstrUsageConfirmationTone1		,
	pstrUsageConfirmationTone2		,
	pstrUsageTonesOff				,
	pstrUsageOutsideRingback		,	
	pstrUsageRinger					
};
const char *ReportDescParserBase::telTitles5			[] PROGMEM = 
{
	pstrUsagePhoneKey0		,
	pstrUsagePhoneKey1		,
	pstrUsagePhoneKey2		,
	pstrUsagePhoneKey3		,
	pstrUsagePhoneKey4		,
	pstrUsagePhoneKey5		,
	pstrUsagePhoneKey6		,
	pstrUsagePhoneKey7		,
	pstrUsagePhoneKey8		,
	pstrUsagePhoneKey9		,
	pstrUsagePhoneKeyStar	,
	pstrUsagePhoneKeyPound	,
	pstrUsagePhoneKeyA		,
	pstrUsagePhoneKeyB		,
	pstrUsagePhoneKeyC		,
	pstrUsagePhoneKeyD		
};
const char *ReportDescParserBase::consTitles0[]	PROGMEM	= 
{
	pstrUsageConsumerControl,		
	pstrUsageNumericKeyPad,		
	pstrUsageProgrammableButton,
	pstrUsageMicrophone,
	pstrUsageHeadphone,
	pstrUsageGraphicEqualizer	
};
const char *ReportDescParserBase::consTitles1[]	PROGMEM	= 
{
	pstrUsagePlus10	,
	pstrUsagePlus100,	
	pstrUsageAMPM	
};
const char *ReportDescParserBase::consTitles2[]	PROGMEM	= 
{
	pstrUsagePower			,
	pstrUsageReset			,
	pstrUsageSleep			,
	pstrUsageSleepAfter		,
	pstrUsageSleepMode		,
	pstrUsageIllumination	,
	pstrUsageFunctionButtons

};
const char *ReportDescParserBase::consTitles3[]	PROGMEM	= 
{
	pstrUsageMenu			,
	pstrUsageMenuPick		,
	pstrUsageMenuUp			,
	pstrUsageMenuDown		,
	pstrUsageMenuLeft		,
	pstrUsageMenuRight		,
	pstrUsageMenuEscape		,
	pstrUsageMenuValueIncrease,
	pstrUsageMenuValueDecrease
};
const char *ReportDescParserBase::consTitles4[]	PROGMEM	= 
{
	pstrUsageDataOnScreen		,
	pstrUsageClosedCaption		,
	pstrUsageClosedCaptionSelect,	
	pstrUsageVCRTV				,
	pstrUsageBroadcastMode		,
	pstrUsageSnapshot			,
	pstrUsageStill				
};
const char *ReportDescParserBase::consTitles5[]	PROGMEM	= 
{
	pstrUsageSelection					,
	pstrUsageAssignSelection			,	
	pstrUsageModeStep					,
	pstrUsageRecallLast					,
	pstrUsageEnterChannel				,
	pstrUsageOrderMovie					,
	pstrUsageChannel					,	
	pstrUsageMediaSelection				,
	pstrUsageMediaSelectComputer		,	
	pstrUsageMediaSelectTV				,
	pstrUsageMediaSelectWWW				,
	pstrUsageMediaSelectDVD				,
	pstrUsageMediaSelectTelephone		,
	pstrUsageMediaSelectProgramGuide	,	
	pstrUsageMediaSelectVideoPhone		,
	pstrUsageMediaSelectGames			,
	pstrUsageMediaSelectMessages		,	
	pstrUsageMediaSelectCD				,
	pstrUsageMediaSelectVCR				,
	pstrUsageMediaSelectTuner			,
	pstrUsageQuit						,
	pstrUsageHelp						,
	pstrUsageMediaSelectTape			,	
	pstrUsageMediaSelectCable			,
	pstrUsageMediaSelectSatellite		,
	pstrUsageMediaSelectSecurity		,	
	pstrUsageMediaSelectHome			,	
	pstrUsageMediaSelectCall			,	
	pstrUsageChannelIncrement			,
	pstrUsageChannelDecrement			,
	pstrUsageMediaSelectSAP				,
	pstrUsagePageReserved				,
	pstrUsageVCRPlus					,	
	pstrUsageOnce						,
	pstrUsageDaily						,
	pstrUsageWeekly						,
	pstrUsageMonthly					
};
const char *ReportDescParserBase::consTitles6[]	PROGMEM	= 
{
	pstrUsagePlay					,	
	pstrUsagePause					,	
	pstrUsageRecord					,
	pstrUsageFastForward			,	
	pstrUsageRewind					,
	pstrUsageScanNextTrack			,	
	pstrUsageScanPreviousTrack		,	
	pstrUsageStop					,	
	pstrUsageEject					,	
	pstrUsageRandomPlay				,	
	pstrUsageSelectDisk				,	
	pstrUsageEnterDisk				,	
	pstrUsageRepeat					,
	pstrUsageTracking					,
	pstrUsageTrackNormal				,	
	pstrUsageSlowTracking				,
	pstrUsageFrameForward				,
	pstrUsageFrameBackwards				,
	pstrUsageMark						,
	pstrUsageClearMark					,
	pstrUsageRepeatFromMark				,
	pstrUsageReturnToMark				,
	pstrUsageSearchMarkForward			,
	pstrUsageSearchMarkBackwards		,	
	pstrUsageCounterReset				,
	pstrUsageShowCounter				,	
	pstrUsageTrackingIncrement			,
	pstrUsageTrackingDecrement			,
	pstrUsageStopEject					,
	pstrUsagePlayPause					,
	pstrUsagePlaySkip					
};
const char *ReportDescParserBase::consTitles7[]	PROGMEM	= 
{
	pstrUsageVolume						,
	pstrUsageBalance					,	
	pstrUsageMute						,
	pstrUsageBass						,
	pstrUsageTreble						,
	pstrUsageBassBoost					,
	pstrUsageSurroundMode				,
	pstrUsageLoudness					,
	pstrUsageMPX						,	
	pstrUsageVolumeIncrement			,	
	pstrUsageVolumeDecrement				
};
const char *ReportDescParserBase::consTitles8[]	PROGMEM	= 
{
	pstrUsageSpeedSelect				,	
	pstrUsagePlaybackSpeed				,
	pstrUsageStandardPlay				,
	pstrUsageLongPlay					,
	pstrUsageExtendedPlay				,
	pstrUsageSlow						
};
const char *ReportDescParserBase::consTitles9[]	PROGMEM	= 
{
	pstrUsageFanEnable					,
	pstrUsageFanSpeed					,
	pstrUsageLightEnable				,	
	pstrUsageLightIlluminationLevel		,
	pstrUsageClimateControlEnable		,
	pstrUsageRoomTemperature			,	
	pstrUsageSecurityEnable				,
	pstrUsageFireAlarm					,
	pstrUsagePoliceAlarm				,	
	pstrUsageProximity					,
	pstrUsageMotion						,
	pstrUsageDuresAlarm					,
	pstrUsageHoldupAlarm					,
	pstrUsageMedicalAlarm				
};
const char *ReportDescParserBase::consTitlesA[]	PROGMEM	= 
{
	pstrUsageBalanceRight				,
	pstrUsageBalanceLeft				,	
	pstrUsageBassIncrement				,
	pstrUsageBassDecrement				,
	pstrUsageTrebleIncrement			,
	pstrUsageTrebleDecrement				
};
const char *ReportDescParserBase::consTitlesB[]	PROGMEM	= 
{
	pstrUsageSpeakerSystem				,
	pstrUsageChannelLeft				,	
	pstrUsageChannelRight				,
	pstrUsageChannelCenter				,
	pstrUsageChannelFront				,
	pstrUsageChannelCenterFront			,
	pstrUsageChannelSide				,	
	pstrUsageChannelSurround			,	
	pstrUsageChannelLowFreqEnhancement	,
	pstrUsageChannelTop					,
	pstrUsageChannelUnknown				
};
const char *ReportDescParserBase::consTitlesC[]	PROGMEM	= 
{
	pstrUsageSubChannel					,
	pstrUsageSubChannelIncrement		,	
	pstrUsageSubChannelDecrement		,	
	pstrUsageAlternateAudioIncrement	,	
	pstrUsageAlternateAudioDecrement		
};
const char *ReportDescParserBase::consTitlesD[]	PROGMEM	= 
{
	pstrUsageApplicationLaunchButtons	,
	pstrUsageALLaunchButtonConfigTool	,
	pstrUsageALProgrammableButton		,
	pstrUsageALConsumerControlConfig	,	
	pstrUsageALWordProcessor			,	
	pstrUsageALTextEditor				,
	pstrUsageALSpreadsheet				,
	pstrUsageALGraphicsEditor			,
	pstrUsageALPresentationApp			,
	pstrUsageALDatabaseApp				,
	pstrUsageALEmailReader				,
	pstrUsageALNewsreader				,
	pstrUsageALVoicemail				,	
	pstrUsageALContactsAddressBook		,
	pstrUsageALCalendarSchedule			,
	pstrUsageALTaskProjectManager		,
	pstrUsageALLogJournalTimecard		,
	pstrUsageALCheckbookFinance			,
	pstrUsageALCalculator				,
	pstrUsageALAVCapturePlayback		,	
	pstrUsageALLocalMachineBrowser		,
	pstrUsageALLANWANBrow				,
	pstrUsageALInternetBrowser			,
	pstrUsageALRemoteNetISPConnect		,
	pstrUsageALNetworkConference		,	
	pstrUsageALNetworkChat				,
	pstrUsageALTelephonyDialer			,
	pstrUsageALLogon					,	
	pstrUsageALLogoff					,
	pstrUsageALLogonLogoff				,
	pstrUsageALTermLockScrSav			,
	pstrUsageALControlPannel			,	
	pstrUsageALCommandLineProcessorRun	,
	pstrUsageALProcessTaskManager		,
	pstrUsageALSelectTaskApplication	,	
	pstrUsageALNextTaskApplication		,
	pstrUsageALPreviousTaskApplication	,
	pstrUsageALPreemptiveHaltTaskApp	,	
	pstrUsageALIntegratedHelpCenter		,
	pstrUsageALDocuments				,	
	pstrUsageALThesaurus				,	
	pstrUsageALDictionary				,
	pstrUsageALDesktop					,
	pstrUsageALSpellCheck				,
	pstrUsageALGrammarCheck				,
	pstrUsageALWirelessStatus			,
	pstrUsageALKeyboardLayout			,
	pstrUsageALVirusProtection			,
	pstrUsageALEncryption				,
	pstrUsageALScreenSaver				,
	pstrUsageALAlarms					,
	pstrUsageALClock					,	
	pstrUsageALFileBrowser				,
	pstrUsageALPowerStatus				,
	pstrUsageALImageBrowser				,
	pstrUsageALAudioBrowser				,
	pstrUsageALMovieBrowser				,
	pstrUsageALDigitalRightsManager		,
	pstrUsageALDigitalWallet			,
	pstrUsagePageReserved				,
	pstrUsageALInstantMessaging			,
	pstrUsageALOEMFeaturesBrowser		,
	pstrUsageALOEMHelp					,
	pstrUsageALOnlineCommunity			,
	pstrUsageALEntertainmentContentBrow	,
	pstrUsageALOnlineShoppingBrowser	,	
	pstrUsageALSmartCardInfoHelp		,	
	pstrUsageALMarketMonitorFinBrowser	,
	pstrUsageALCustomCorpNewsBrowser		,
	pstrUsageALOnlineActivityBrowser		,
	pstrUsageALResearchSearchBrowser		,
	pstrUsageALAudioPlayer				
};
const char *ReportDescParserBase::consTitlesE[]	PROGMEM	= 
{
	pstrUsageGenericGUIAppControls		,
	pstrUsageACNew						,
	pstrUsageACOpen						,
	pstrUsageACClose					,	
	pstrUsageACExit						,
	pstrUsageACMaximize					,
	pstrUsageACMinimize					,
	pstrUsageACSave						,
	pstrUsageACPrint					,	
	pstrUsageACProperties				,
	pstrUsageACUndo						,
	pstrUsageACCopy						,
	pstrUsageACCut						,
	pstrUsageACPaste					,	
	pstrUsageACSelectAll				,	
	pstrUsageACFind						,
	pstrUsageACFindAndReplace			,
	pstrUsageACSearch					,
	pstrUsageACGoto						,
	pstrUsageACHome						,
	pstrUsageACBack						,
	pstrUsageACForward					,
	pstrUsageACStop						,
	pstrUsageACRefresh					,
	pstrUsageACPreviousLink				,
	pstrUsageACNextLink					,
	pstrUsageACBookmarks				,	
	pstrUsageACHistory					,
	pstrUsageACSubscriptions			,	
	pstrUsageACZoomIn					,
	pstrUsageACZoomOut					,
	pstrUsageACZoom						,
	pstrUsageACFullScreenView			,
	pstrUsageACNormalView				,
	pstrUsageACViewToggle				,
	pstrUsageACScrollUp					,
	pstrUsageACScrollDown				,
	pstrUsageACScroll					,
	pstrUsageACPanLeft					,
	pstrUsageACPanRight					,
	pstrUsageACPan						,
	pstrUsageACNewWindow				,	
	pstrUsageACTileHoriz				,	
	pstrUsageACTileVert					,
	pstrUsageACFormat					,
	pstrUsageACEdit						,
	pstrUsageACBold						,
	pstrUsageACItalics					,
	pstrUsageACUnderline				,	
	pstrUsageACStrikethrough			,	
	pstrUsageACSubscript				,	
	pstrUsageACSuperscript				,
	pstrUsageACAllCaps					,
	pstrUsageACRotate					,
	pstrUsageACResize					,
	pstrUsageACFlipHorizontal			,
	pstrUsageACFlipVertical				,
	pstrUsageACMirrorHorizontal			,
	pstrUsageACMirrorVertical			,
	pstrUsageACFontSelect				,
	pstrUsageACFontColor				,	
	pstrUsageACFontSize					,
	pstrUsageACJustifyLeft				,
	pstrUsageACJustifyCenterH			,
	pstrUsageACJustifyRight				,
	pstrUsageACJustifyBlockH			,	
	pstrUsageACJustifyTop				,
	pstrUsageACJustifyCenterV			,
	pstrUsageACJustifyBottom			,	
	pstrUsageACJustifyBlockV			,	
	pstrUsageACIndentDecrease			,
	pstrUsageACIndentIncrease			,
	pstrUsageACNumberedList				,
	pstrUsageACRestartNumbering			,
	pstrUsageACBulletedList				,
	pstrUsageACPromote					,
	pstrUsageACDemote					,
	pstrUsageACYes						,
	pstrUsageACNo						,
	pstrUsageACCancel					,
	pstrUsageACCatalog					,
	pstrUsageACBuyChkout				,	
	pstrUsageACAddToCart				,	
	pstrUsageACExpand					,
	pstrUsageACExpandAll				,	
	pstrUsageACCollapse					,
	pstrUsageACCollapseAll				,
	pstrUsageACPrintPreview				,
	pstrUsageACPasteSpecial				,
	pstrUsageACInsertMode				,
	pstrUsageACDelete					,
	pstrUsageACLock						,
	pstrUsageACUnlock					,
	pstrUsageACProtect					,
	pstrUsageACUnprotect				,	
	pstrUsageACAttachComment			,	
	pstrUsageACDeleteComment			,	
	pstrUsageACViewComment				,
	pstrUsageACSelectWord				,
	pstrUsageACSelectSentence			,
	pstrUsageACSelectParagraph			,
	pstrUsageACSelectColumn				,
	pstrUsageACSelectRow				,	
	pstrUsageACSelectTable				,
	pstrUsageACSelectObject				,
	pstrUsageACRedoRepeat				,
	pstrUsageACSort						,
	pstrUsageACSortAscending			,	
	pstrUsageACSortDescending			,
	pstrUsageACFilter					,
	pstrUsageACSetClock					,
	pstrUsageACViewClock				,	
	pstrUsageACSelectTimeZone			,
	pstrUsageACEditTimeZone				,
	pstrUsageACSetAlarm					,
	pstrUsageACClearAlarm				,
	pstrUsageACSnoozeAlarm				,
	pstrUsageACResetAlarm				,
	pstrUsageACSyncronize				,
	pstrUsageACSendReceive				,
	pstrUsageACSendTo					,
	pstrUsageACReply					,	
	pstrUsageACReplyAll					,
	pstrUsageACForwardMessage			,
	pstrUsageACSend						,
	pstrUsageACAttachFile				,
	pstrUsageACUpload					,
	pstrUsageACDownload					,
	pstrUsageACSetBorders				,
	pstrUsageACInsertRow				,	
	pstrUsageACInsertColumn				,
	pstrUsageACInsertFile				,
	pstrUsageACInsertPicture			,	
	pstrUsageACInsertObject				,
	pstrUsageACInsertSymbol				,
	pstrUsageACSaveAndClose				,
	pstrUsageACRename					,
	pstrUsageACMerge					,
	pstrUsageACSplit					,
	pstrUsageACDistributeHorizontaly	,
	pstrUsageACDistributeVerticaly		
};
const char *ReportDescParserBase::digitTitles0[] PROGMEM = 
{
	pstrUsageDigitizer					,
	pstrUsagePen						,	
	pstrUsageLightPen					,
	pstrUsageTouchScreen				,	
	pstrUsageTouchPad					,
	pstrUsageWhiteBoard					,
	pstrUsageCoordinateMeasuringMachine	,
	pstrUsage3DDigitizer				,	
	pstrUsageStereoPlotter				,
	pstrUsageArticulatedArm				,
	pstrUsageArmature					,
	pstrUsageMultiplePointDigitizer		,
	pstrUsageFreeSpaceWand				
};
const char *ReportDescParserBase::digitTitles1[] PROGMEM = 
{
	pstrUsageStylus						,
	pstrUsagePuck						,
	pstrUsageFinger						

};
const char *ReportDescParserBase::digitTitles2[] PROGMEM = 
{
	pstrUsageTipPressure			,	
	pstrUsageBarrelPressure			,
	pstrUsageInRange				,	
	pstrUsageTouch					,
	pstrUsageUntouch				,	
	pstrUsageTap					,	
	pstrUsageQuality				,	
	pstrUsageDataValid				,
	pstrUsageTransducerIndex		,	
	pstrUsageTabletFunctionKeys		,
	pstrUsageProgramChangeKeys		,
	pstrUsageBatteryStrength		,	
	pstrUsageInvert					,
	pstrUsageXTilt					,
	pstrUsageYTilt					,
	pstrUsageAzimuth				,	
	pstrUsageAltitude				,
	pstrUsageTwist					,
	pstrUsageTipSwitch				,
	pstrUsageSecondaryTipSwitch		,
	pstrUsageBarrelSwitch			,
	pstrUsageEraser					,
	pstrUsageTabletPick				
};
const char *ReportDescParserBase::aplphanumTitles0[]	PROGMEM =
{
	pstrUsageAlphanumericDisplay,
	pstrUsageBitmappedDisplay	
};
const char *ReportDescParserBase::aplphanumTitles1[]	PROGMEM =
{
	pstrUsageDisplayAttributesReport	,	
	pstrUsageASCIICharacterSet			,
	pstrUsageDataReadBack				,
	pstrUsageFontReadBack				,
	pstrUsageDisplayControlReport		,
	pstrUsageClearDisplay				,
	pstrUsageDisplayEnable				,
	pstrUsageScreenSaverDelay			,
	pstrUsageScreenSaverEnable			,
	pstrUsageVerticalScroll				,
	pstrUsageHorizontalScroll			,
	pstrUsageCharacterReport			,	
	pstrUsageDisplayData				,	
	pstrUsageDisplayStatus				,
	pstrUsageStatusNotReady				,
	pstrUsageStatusReady				,	
	pstrUsageErrorNotALoadableCharacter	,
	pstrUsageErrorFotDataCanNotBeRead	,
	pstrUsageCursorPositionReport		,
	pstrUsageRow						,	
	pstrUsageColumn						,
	pstrUsageRows						,
	pstrUsageColumns					,	
	pstrUsageCursorPixelPosition		,	
	pstrUsageCursorMode					,
	pstrUsageCursorEnable				,
	pstrUsageCursorBlink				,	
	pstrUsageFontReport					,
	pstrUsageFontData					,
	pstrUsageCharacterWidth				,
	pstrUsageCharacterHeight			,	
	pstrUsageCharacterSpacingHorizontal	,
	pstrUsageCharacterSpacingVertical	,
	pstrUsageUnicodeCharset				,
	pstrUsageFont7Segment				,
	pstrUsage7SegmentDirectMap			,
	pstrUsageFont14Segment				,
	pstrUsage14SegmentDirectMap			,
	pstrUsageDisplayBrightness			,
	pstrUsageDisplayContrast			,	
	pstrUsageCharacterAttribute			,
	pstrUsageAttributeReadback			,
	pstrUsageAttributeData				,
	pstrUsageCharAttributeEnhance		,
	pstrUsageCharAttributeUnderline		,
	pstrUsageCharAttributeBlink			
};
const char *ReportDescParserBase::aplphanumTitles2[]	PROGMEM =
{
	pstrUsageBitmapSizeX				,	
	pstrUsageBitmapSizeY				,
	pstrUsagePageReserved				,
	pstrUsageBitDepthFormat				,
	pstrUsageDisplayOrientation			,
	pstrUsagePaletteReport				,
	pstrUsagePaletteDataSize			,	
	pstrUsagePaletteDataOffset			,
	pstrUsagePaletteData				,	
	pstrUsageBlitReport					,
	pstrUsageBlitRectangleX1			,	
	pstrUsageBlitRectangleY1			,	
	pstrUsageBlitRectangleX2			,	
	pstrUsageBlitRectangleY2			,	
	pstrUsageBlitData					,
	pstrUsageSoftButton					,
	pstrUsageSoftButtonID				,
	pstrUsageSoftButtonSide				,
	pstrUsageSoftButtonOffset1			,
	pstrUsageSoftButtonOffset2			,
	pstrUsageSoftButtonReport			
};
const char *ReportDescParserBase::medInstrTitles0[] PROGMEM =
{
	pstrUsageVCRAcquisition				,
	pstrUsageFreezeThaw					,
	pstrUsageClipStore					,
	pstrUsageUpdate						,
	pstrUsageNext						,
	pstrUsageSave						,
	pstrUsagePrint						,
	pstrUsageMicrophoneEnable			
};
const char *ReportDescParserBase::medInstrTitles1[] PROGMEM =
{
	pstrUsageCine						,
	pstrUsageTransmitPower				,
	pstrUsageVolume						,
	pstrUsageFocus						,
	pstrUsageDepth						
};
const char *ReportDescParserBase::medInstrTitles2[] PROGMEM =
{
	pstrUsageSoftStepPrimary		,	
	pstrUsageSoftStepSecondary		
};
const char *ReportDescParserBase::medInstrTitles3[] PROGMEM =
{
	pstrUsageZoomSelect					,
	pstrUsageZoomAdjust					,
	pstrUsageSpectralDopplerModeSelect	,
	pstrUsageSpectralDopplerModeAdjust	,
	pstrUsageColorDopplerModeSelect		,
	pstrUsageColorDopplerModeAdjust		,
	pstrUsageMotionModeSelect			,
	pstrUsageMotionModeAdjust			,
	pstrUsage2DModeSelect				,
	pstrUsage2DModeAdjust				
};
const char *ReportDescParserBase::medInstrTitles4[] PROGMEM =
{
	pstrUsageSoftControlSelect			,
	pstrUsageSoftControlAdjust			
};

void ReportDescParserBase::Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset)
{
	uint16_t	cntdn	= (uint16_t)len;
	uint8_t		*p		= (uint8_t*)pbuf; 


	totalSize = 0;

	while(cntdn)
	{
		//Serial.println("");
		//PrintHex<uint16_t>(offset + len - cntdn);
		//Serial.print(":");

		ParseItem(&p, &cntdn);

		//if (ParseItem(&p, &cntdn))
		//	return;
	}
	USBTRACE2("Total:", totalSize);
}

void ReportDescParserBase::PrintValue(uint8_t *p, uint8_t len)
{
	Notify(PSTR("("));
	for (; len; p++, len--)
		PrintHex<uint8_t>(*p);
	Notify(PSTR(")"));
}

void ReportDescParserBase::PrintByteValue(uint8_t data)
{
	Notify(PSTR("("));
	PrintHex<uint8_t>(data);
	Notify(PSTR(")"));
}

void ReportDescParserBase::PrintItemTitle(uint8_t prefix)
{
	switch (prefix & (TYPE_MASK | TAG_MASK))
	{
	case (TYPE_GLOBAL | TAG_GLOBAL_PUSH):
		Notify(PSTR("\r\nPush"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_POP):
		Notify(PSTR("\r\nPop"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
		Notify(PSTR("\r\nUsage Page"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMIN):
		Notify(PSTR("\r\nLogical Min"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMAX):
		Notify(PSTR("\r\nLogical Max"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMIN):
		Notify(PSTR("\r\nPhysical Min"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMAX):
		Notify(PSTR("\r\nPhysical Max"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_UNITEXP):
		Notify(PSTR("\r\nUnit Exp"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_UNIT):
		Notify(PSTR("\r\nUnit"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
		Notify(PSTR("\r\nReport Size"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
		Notify(PSTR("\r\nReport Count"));
		break;
	case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
		Notify(PSTR("\r\nReport Id"));
		break;
	case (TYPE_LOCAL | TAG_LOCAL_USAGE):
		Notify(PSTR("\r\nUsage"));
		break;
	case (TYPE_LOCAL | TAG_LOCAL_USAGEMIN):
		Notify(PSTR("\r\nUsage Min"));
		break;
	case (TYPE_LOCAL | TAG_LOCAL_USAGEMAX):
		Notify(PSTR("\r\nUsage Max"));
		break;
	case (TYPE_MAIN | TAG_MAIN_COLLECTION):
		Notify(PSTR("\r\nCollection"));
		break;
	case (TYPE_MAIN | TAG_MAIN_ENDCOLLECTION):
		Notify(PSTR("\r\nEnd Collection"));
		break;
	case (TYPE_MAIN | TAG_MAIN_INPUT):
		Notify(PSTR("\r\nInput"));
		break;
	case (TYPE_MAIN | TAG_MAIN_OUTPUT):
		Notify(PSTR("\r\nOutput"));
		break;
	case (TYPE_MAIN | TAG_MAIN_FEATURE):
		Notify(PSTR("\r\nFeature"));
		break;
	} // switch (**pp & (TYPE_MASK | TAG_MASK))
}
uint8_t ReportDescParserBase::ParseItem(uint8_t **pp, uint16_t *pcntdn)
{
	uint8_t	ret = enErrorSuccess;

	switch (itemParseState)
	{
	case 0:
		if (**pp == HID_LONG_ITEM_PREFIX)
			USBTRACE("\r\nLONG\r\n");
		else
		{
			uint8_t size	= ((**pp) & DATA_SIZE_MASK);

			itemPrefix		= (**pp);
			itemSize		= 1 + ((size == DATA_SIZE_4) ? 4 : size);

			PrintItemTitle(itemPrefix);
		}
		(*pp)		++;
		(*pcntdn)	--;
		itemSize	--;
		itemParseState	= 1;

		if (!itemSize)
			break;

		if (!pcntdn)
			return enErrorIncomplete;
	case 1:
		//USBTRACE2("\r\niSz:",itemSize);

        theBuffer.valueSize = itemSize;
		valParser.Initialize(&theBuffer);
		itemParseState	= 2;
	case 2:
		if (!valParser.Parse(pp, pcntdn))
            return enErrorIncomplete;
		itemParseState	= 3;
	case 3:
		{
		uint8_t data = *((uint8_t*)varBuffer);

		switch (itemPrefix & (TYPE_MASK | TAG_MASK))
		{
		case (TYPE_LOCAL  | TAG_LOCAL_USAGE):
			if (pfUsage)
				if (theBuffer.valueSize > 1)
					pfUsage(*((uint16_t*)varBuffer));
				else
					pfUsage(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
			rptSize = data;
			PrintByteValue(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
			rptCount = data;
			PrintByteValue(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMIN):
		case (TYPE_GLOBAL | TAG_GLOBAL_LOGICALMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMIN):
		case (TYPE_GLOBAL | TAG_GLOBAL_PHYSMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMIN):
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMAX):
		case (TYPE_GLOBAL | TAG_GLOBAL_UNITEXP):
		case (TYPE_GLOBAL | TAG_GLOBAL_UNIT):
			PrintValue(varBuffer, theBuffer.valueSize);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_PUSH):
		case (TYPE_GLOBAL | TAG_GLOBAL_POP):
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
			SetUsagePage(data);
			PrintUsagePage(data);
			PrintByteValue(data);
			break;
		case (TYPE_MAIN   | TAG_MAIN_COLLECTION):
		case (TYPE_MAIN   | TAG_MAIN_ENDCOLLECTION):
			switch (data)
			{
			case 0x00:
				Notify(PSTR(" Physical"));
				break;
			case 0x01:
				Notify(PSTR(" Application"));
				break;
			case 0x02:
				Notify(PSTR(" Logical"));
				break;
			case 0x03:
				Notify(PSTR(" Report"));
				break;
			case 0x04:
				Notify(PSTR(" Named Array"));
				break;
			case 0x05:
				Notify(PSTR(" Usage Switch"));
				break;
			case 0x06:
				Notify(PSTR(" Usage Modifier"));
				break;
			default:
				Notify(PSTR(" Vendor Defined("));
				PrintHex<uint8_t>(data);
				Notify(PSTR(")"));
			}
			break;
		case (TYPE_MAIN   | TAG_MAIN_INPUT):
		case (TYPE_MAIN   | TAG_MAIN_OUTPUT):
		case (TYPE_MAIN   | TAG_MAIN_FEATURE):
			totalSize	+= (uint16_t)rptSize * (uint16_t)rptCount;
			rptSize		= 0;
			rptCount	= 0;
			Notify(PSTR("("));
			PrintBin<uint8_t>(data);
			Notify(PSTR(")"));
			break;
		} // switch (**pp & (TYPE_MASK | TAG_MASK))
		}
	} // switch (itemParseState)
	itemParseState	= 0;
	return enErrorSuccess;
}

ReportDescParserBase::UsagePageFunc		ReportDescParserBase::usagePageFunctions[] /*PROGMEM*/ =
{
	&ReportDescParserBase::PrintGenericDesktopPageUsage,
	&ReportDescParserBase::PrintSimulationControlsPageUsage,
	&ReportDescParserBase::PrintVRControlsPageUsage,
	&ReportDescParserBase::PrintSportsControlsPageUsage,
	&ReportDescParserBase::PrintGameControlsPageUsage,
	&ReportDescParserBase::PrintGenericDeviceControlsPageUsage,
	NULL,		// Keyboard/Keypad
	&ReportDescParserBase::PrintLEDPageUsage,
	&ReportDescParserBase::PrintButtonPageUsage,
	&ReportDescParserBase::PrintOrdinalPageUsage,
	&ReportDescParserBase::PrintTelephonyPageUsage,
	&ReportDescParserBase::PrintConsumerPageUsage,
	&ReportDescParserBase::PrintDigitizerPageUsage,
	NULL,		// Reserved
	NULL,		// PID
	NULL		// Unicode
};

void ReportDescParserBase::SetUsagePage(uint16_t page)
{
	pfUsage = NULL;

	if (page > 0x00 && page < 0x11)
		pfUsage = /*(UsagePageFunc)pgm_read_word*/(usagePageFunctions[page - 1]);
	//else if (page > 0x7f && page < 0x84)
	//	Notify(pstrUsagePageMonitor);
	//else if (page > 0x83 && page < 0x8c)
	//	Notify(pstrUsagePagePower);
	//else if (page > 0x8b && page < 0x92)
	//	Notify((char*)pgm_read_word(&usagePageTitles1[page - 0x8c]));
	//else if (page > 0xfeff && page <= 0xffff)
	//	Notify(pstrUsagePageVendorDefined);
	else
		switch (page)
		{
		case 0x14:
			pfUsage = &ReportDescParserBase::PrintAlphanumDisplayPageUsage;
			break;
		case 0x40:
			pfUsage = &ReportDescParserBase::PrintMedicalInstrumentPageUsage;
			break;
		}
}

void ReportDescParserBase::PrintUsagePage(uint16_t page)
{
	Notify(pstrSpace);

	if (page > 0x00 && page < 0x11)
		Notify((char*)pgm_read_word(&usagePageTitles0[page - 1]));
	else if (page > 0x7f && page < 0x84)
		Notify(pstrUsagePageMonitor);
	else if (page > 0x83 && page < 0x8c)
		Notify(pstrUsagePagePower);
	else if (page > 0x8b && page < 0x92)
		Notify((char*)pgm_read_word(&usagePageTitles1[page - 0x8c]));
	else if (page > 0xfeff && page <= 0xffff)
		Notify(pstrUsagePageVendorDefined);
	else
		switch (page)
		{
		case 0x14:
			Notify(pstrUsagePageAlphaNumericDisplay);
			break;
		case 0x40:
			Notify(pstrUsagePageMedicalInstruments);
			break;
		default:
			Notify(pstrUsagePageUndefined);
		}
}

void ReportDescParserBase::PrintButtonPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	Notify(PSTR("Btn"));
	Serial.print(usage, DEC);
}

void ReportDescParserBase::PrintOrdinalPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	Notify(PSTR("Inst"));
	Serial.print(usage, DEC);
}

void ReportDescParserBase::PrintGenericDesktopPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0a)
		Notify((char*)pgm_read_word(&genDesktopTitles0[usage - 1]));
	else if (usage > 0x2f && usage < 0x49)
		Notify((char*)pgm_read_word(&genDesktopTitles1[usage - 0x30]));
	else if (usage > 0x7f && usage < 0x94)
		Notify((char*)pgm_read_word(&genDesktopTitles2[usage - 0x80]));
	else if (usage > 0x9f && usage < 0xa9)
		Notify((char*)pgm_read_word(&genDesktopTitles3[usage - 0xa0]));
	else if (usage > 0xaf && usage < 0xb8)
		Notify((char*)pgm_read_word(&genDesktopTitles4[usage - 0xb0]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintSimulationControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0d)
		Notify((char*)pgm_read_word(&simuTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x26)
		Notify((char*)pgm_read_word(&simuTitles1[usage - 0x20]));
	else if (usage > 0xaf && usage < 0xd1)
		Notify((char*)pgm_read_word(&simuTitles2[usage - 0xb0]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintVRControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x0b)
		Notify((char*)pgm_read_word(&vrTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x22)
		Notify((char*)pgm_read_word(&vrTitles1[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintSportsControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x05)
		Notify((char*)pgm_read_word(&sportsCtrlTitles0[usage - 1]));
	else if (usage > 0x2f && usage < 0x3a)
		Notify((char*)pgm_read_word(&sportsCtrlTitles1[usage - 0x30]));
	else if (usage > 0x4f && usage < 0x64)
		Notify((char*)pgm_read_word(&sportsCtrlTitles2[usage - 0x50]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintGameControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x04)
		Notify((char*)pgm_read_word(&gameTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x3a)
		Notify((char*)pgm_read_word(&gameTitles1[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintGenericDeviceControlsPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x1f && usage < 0x27)
		Notify((char*)pgm_read_word(&genDevCtrlTitles[usage - 0x20]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintLEDPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x4e)
		Notify((char*)pgm_read_word(&ledTitles[usage - 1]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintTelephonyPageUsage(uint16_t usage)
{
	Notify(pstrSpace);

	if (usage > 0x00 && usage < 0x08)
		Notify((char*)pgm_read_word(&telTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x32)
		Notify((char*)pgm_read_word(&telTitles1[usage - 0x1f]));
	else if (usage > 0x4f && usage < 0x54)
		Notify((char*)pgm_read_word(&telTitles2[usage - 0x4f]));
	else if (usage > 0x6f && usage < 0x75)
		Notify((char*)pgm_read_word(&telTitles3[usage - 0x6f]));
	else if (usage > 0x8f && usage < 0x9f)
		Notify((char*)pgm_read_word(&telTitles4[usage - 0x8f]));
	else if (usage > 0xaf && usage < 0xc0)
		Notify((char*)pgm_read_word(&telTitles5[usage - 0xaf]));
	else
		Notify(pstrUsagePageUndefined);
}
	
void ReportDescParserBase::PrintConsumerPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x07)
		Notify((char*)pgm_read_word(&consTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x23)
		Notify((char*)pgm_read_word(&consTitles1[usage - 0x1f]));
	else if (usage > 0x2f && usage < 0x37)
		Notify((char*)pgm_read_word(&consTitles2[usage - 0x2f]));
	else if (usage > 0x3f && usage < 0x49)
		Notify((char*)pgm_read_word(&consTitles3[usage - 0x3f]));
	else if (usage > 0x5f && usage < 0x67)
		Notify((char*)pgm_read_word(&consTitles4[usage - 0x5f]));
	else if (usage > 0x7f && usage < 0xa5)
		Notify((char*)pgm_read_word(&consTitles5[usage - 0x7f]));
	else if (usage > 0xaf && usage < 0xcf)
		Notify((char*)pgm_read_word(&consTitles6[usage - 0xaf]));
	else if (usage > 0xdf && usage < 0xeb)
		Notify((char*)pgm_read_word(&consTitles7[usage - 0xdf]));
	else if (usage > 0xef && usage < 0xf6)
		Notify((char*)pgm_read_word(&consTitles8[usage - 0xef]));
	else if (usage > 0xff && usage < 0x10e)
		Notify((char*)pgm_read_word(&consTitles9[usage - 0xff]));
	else if (usage > 0x14f && usage < 0x156)
		Notify((char*)pgm_read_word(&consTitlesA[usage - 0x14f]));
	else if (usage > 0x15f && usage < 0x16b)
		Notify((char*)pgm_read_word(&consTitlesB[usage - 0x15f]));
	else if (usage > 0x16f && usage < 0x175)
		Notify((char*)pgm_read_word(&consTitlesC[usage - 0x16f]));
	else if (usage > 0x17f && usage < 0x1c8)
		Notify((char*)pgm_read_word(&consTitlesD[usage - 0x17f]));
	else if (usage > 0x1ff && usage < 0x29d)
		Notify((char*)pgm_read_word(&consTitlesE[usage - 0x1ff]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintDigitizerPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x0e)
		Notify((char*)pgm_read_word(&digitTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x23)
		Notify((char*)pgm_read_word(&digitTitles1[usage - 0x1f]));
	else if (usage > 0x2f && usage < 0x47)
		Notify((char*)pgm_read_word(&digitTitles2[usage - 0x2f]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintAlphanumDisplayPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage > 0x00 && usage < 0x03)
		Notify((char*)pgm_read_word(&aplphanumTitles0[usage - 1]));
	else if (usage > 0x1f && usage < 0x4e)
		Notify((char*)pgm_read_word(&aplphanumTitles1[usage - 0x1f]));
	else if (usage > 0x7f && usage < 0x96)
		Notify((char*)pgm_read_word(&digitTitles2[usage - 0x80]));
	else
		Notify(pstrUsagePageUndefined);
}

void ReportDescParserBase::PrintMedicalInstrumentPageUsage(uint16_t usage)
{
	Notify(pstrSpace);
	
	if (usage == 1)
		Notify(pstrUsageMedicalUltrasound);
	else if (usage > 0x1f && usage < 0x28)
		Notify((char*)pgm_read_word(&medInstrTitles0[usage - 0x1f]));
	else if (usage > 0x3f && usage < 0x45)
		Notify((char*)pgm_read_word(&medInstrTitles1[usage - 0x40]));
	else if (usage > 0x5f && usage < 0x62)
		Notify((char*)pgm_read_word(&medInstrTitles2[usage - 0x60]));
	else if (usage == 0x70)
		Notify(pstrUsageDepthGainCompensation);
	else if (usage > 0x7f && usage < 0x8a)
		Notify((char*)pgm_read_word(&medInstrTitles3[usage - 0x80]));
	else if (usage > 0x9f && usage < 0xa2)
		Notify((char*)pgm_read_word(&medInstrTitles4[usage - 0xa0]));
	else
		Notify(pstrUsagePageUndefined);
}





uint8_t ReportDescParser2::ParseItem(uint8_t **pp, uint16_t *pcntdn)
{
	uint8_t	ret = enErrorSuccess;

	switch (itemParseState)
	{
	case 0:
		if (**pp == HID_LONG_ITEM_PREFIX)
			USBTRACE("\r\nLONG\r\n");
		else
		{
			uint8_t size	= ((**pp) & DATA_SIZE_MASK);

			itemPrefix		= (**pp);
			itemSize		= 1 + ((size == DATA_SIZE_4) ? 4 : size);

			//PrintItemTitle(itemPrefix);
		}
		(*pp)		++;
		(*pcntdn)	--;
		itemSize	--;
		itemParseState	= 1;

		if (!itemSize)
			break;

		if (!pcntdn)
			return enErrorIncomplete;
	case 1:
		//USBTRACE2("\r\niSz:",itemSize);

        theBuffer.valueSize = itemSize;
		valParser.Initialize(&theBuffer);
		itemParseState	= 2;
	case 2:
		if (!valParser.Parse(pp, pcntdn))
            return enErrorIncomplete;
		itemParseState	= 3;
	case 3:
		{
		uint8_t data = *((uint8_t*)varBuffer);

		switch (itemPrefix & (TYPE_MASK | TAG_MASK))
		{
		case (TYPE_LOCAL  | TAG_LOCAL_USAGE):
			if (pfUsage)
				if (theBuffer.valueSize > 1)
					pfUsage(*((uint16_t*)varBuffer));
				else
					pfUsage(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
			rptSize = data;
			//PrintByteValue(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
			rptCount = data;
			//PrintByteValue(data);
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
			rptId = data;
			break;
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMIN):
			useMin = data;
			break;
		case (TYPE_LOCAL  | TAG_LOCAL_USAGEMAX):
			useMax = data;
			break;
		case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
			SetUsagePage(data);
			//PrintUsagePage(data);
			//PrintByteValue(data);
			break;
		case (TYPE_MAIN   | TAG_MAIN_OUTPUT):
		case (TYPE_MAIN   | TAG_MAIN_FEATURE):
			rptSize		= 0;
			rptCount	= 0;
			useMin		= 0;
			useMax		= 0;
			break;
		case (TYPE_MAIN   | TAG_MAIN_INPUT):
			OnInputItem(data);

			totalSize	+= (uint16_t)rptSize * (uint16_t)rptCount;

			rptSize		= 0;
			rptCount	= 0;
			useMin		= 0;
			useMax		= 0;
			break;
		} // switch (**pp & (TYPE_MASK | TAG_MASK))
		}
	} // switch (itemParseState)
	itemParseState	= 0;
	return enErrorSuccess;
}

void ReportDescParser2::OnInputItem(uint8_t itm)
{
	uint8_t		byte_offset		= (totalSize >> 3);		// calculate offset to the next unhandled byte i = (int)(totalCount / 8);
	uint32_t	tmp				= (byte_offset << 3);
	uint8_t		bit_offset		= totalSize - tmp;			// number of bits in the current byte already handled
	uint8_t		*p				= pBuf + byte_offset;		// current byte pointer

	//Serial.print(" tS:");
	//PrintHex<uint32_t>(totalSize);

	//Serial.print(" byO:");
	//PrintHex<uint8_t>(byte_offset);

	//Serial.print(" biO:");
	//PrintHex<uint8_t>(bit_offset);

	//Serial.print(" rSz:");
	//PrintHex<uint8_t>(rptSize);

	//Serial.print(" rCn:");
	//PrintHex<uint8_t>(rptCount);

	if (bit_offset)
		*p >>= bit_offset;

	uint8_t		usage = useMin;

	bool print_usemin_usemax = ( (useMin < useMax) && ((itm & 3) == 2) && pfUsage) ? true : false;

	uint8_t		bits_of_byte = 8;

	for (uint8_t i=0; i<rptCount; i++, usage++)
	{
		union
		{
			uint8_t		bResult[4];
			uint16_t	wResult[2];
			uint32_t	dwResult;
		}	result;

		result.dwResult = 0;

		uint8_t		mask = 0;

		// bits_left		- number of bits in the field left to process
		// bits_of_byte		- number of bits in current byte left to process
		// bits_to_copy		- number of bits to copy to result buffer

		for (uint8_t bits_left=rptSize, bits_to_copy=0; bits_left; 
			bits_left -= bits_to_copy)
		{
			bits_to_copy = (bits_left > 8) ? 8 : (bits_left > bits_of_byte) ? bits_of_byte : bits_left;

			result.dwResult <<= bits_to_copy;					// Result buffer is shifted by the number of bits to be copied into it

			if (bits_to_copy == 8)
			{
				result.bResult[0]	= *p;
				bits_of_byte		= 8;
				p ++;
				continue;
			}

			uint8_t		val = *p;

			val >>= (8 - bits_of_byte);							// Shift by the number of bits already processed

			//Serial.print(" sh:");
			//PrintHex<uint8_t>(8 - bits_of_byte);

			mask = 0;

			for (uint8_t j=bits_to_copy; j; j--)
			{
				mask <<= 1;
				mask |=  1;
			}

			//Serial.print(" msk:");
			//PrintHex<uint8_t>(mask);

			result.bResult[0] = (result.bResult[0] | (val & mask));

			//Serial.print(" res:");
			//PrintHex<uint8_t>(result.bResult[0]);

			//Serial.print(" b2c:");
			//PrintHex<uint8_t>(bits_to_copy);

			bits_of_byte -= bits_to_copy;

			//Serial.print(" bob:");
			//PrintHex<uint8_t>(bits_of_byte);
		}

		PrintByteValue(result.bResult[0]);
	}
	Serial.println("");
}

void UniversalReportParser::Parse(HID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
	ReportDescParser2	prs(len, buf);

	uint8_t ret = hid->GetReportDescr(0, &prs);

    if (ret)
        ErrorMessage<uint8_t>(PSTR("GetReportDescr-2"), ret);
}