//
//  V2Overrides.cpp
//  GenericUSBXHCI
//
//  Created by Zenith432 on December 26th, 2012.
//  Copyright (c) 2012-2013 Zenith432. All rights reserved.
//

#include "GenericUSBXHCI.h"
#include "Isoch.h"

#define CLASS GenericUSBXHCI
#define super IOUSBControllerV3

#pragma mark -
#pragma mark IOUSBControllerV2 Overrides
#pragma mark -

IOReturn CLASS::ConfigureDeviceZero(UInt8 maxPacketSize, UInt8 speed, USBDeviceAddress hub, int port)
{
	uint16_t _port = static_cast<uint16_t>(port);

	if (hub == _hub2Address || hub == _hub3Address) {
		_port = PortNumberProtocolToCanonical(_port, (hub == _hub3Address ? kUSBDeviceSpeedSuper : kUSBDeviceSpeedHigh));
		if (_port >= _rootHubNumPorts)
			return kIOReturnNoDevice;
		++_port;
	}
	_deviceZero.PortOnHub = _port;
	_deviceZero.HubAddress = hub;
	return super::ConfigureDeviceZero(maxPacketSize, speed, hub, _port);
}

IOReturn CLASS::UIMHubMaintenance(USBDeviceAddress highSpeedHub, UInt32 highSpeedPort, UInt32 command, UInt32 flags)
{
	if (command == kUSBHSHubCommandRemoveHub)
		return kIOReturnSuccess;
	if (command != kUSBHSHubCommandAddHub)
		return kIOReturnBadArgument;
	return configureHub(highSpeedHub, flags);
}

IOReturn CLASS::UIMSetTestMode(UInt32 mode, UInt32 port)
{
	switch (mode) {
		case 0U:
		case 1U:
		case 2U:
		case 3U:
		case 4U:
		case 5U:
			if (!_inTestMode)
				break;
			return PlacePortInMode(port, mode);
		case 10U:
			return EnterTestMode();
		case 11U:
			return LeaveTestMode();
	}
	return kIOReturnInternalError;
}

UInt64 CLASS::GetMicroFrameNumber(void)
{
	uint64_t counter1, counter2;
	uint32_t sts, mfIndex, count;
	sts = Read32Reg(&_pXHCIOperationalRegisters->USBSts);
	if (m_invalid_regspace || (sts & XHCI_STS_HCH))
		return 0ULL;
	for (count = 0U; count < 100U; ++count) {
		if (count)
			IODelay(126U);
		counter1 = _millsecondCounter;
		mfIndex = Read32Reg(&_pXHCIRuntimeRegisters->MFIndex);
		counter2 = _millsecondCounter;
		if (m_invalid_regspace)
			return 0ULL;
		mfIndex &= XHCI_MFINDEX_MASK;
		if (counter1 == counter2 && mfIndex)
			break;
	}
	if (count == 100U)
		return 0ULL;
	return (counter1 << 3) + mfIndex;
}

IOReturn CLASS::UIMCreateIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize, UInt8 direction,
									   USBDeviceAddress highSpeedHub, int highSpeedPort, UInt8 interval)
{
	return CreateIsochEndpoint(functionAddress, endpointNumber, maxPacketSize, direction, interval, 0U);
}

IOUSBControllerIsochEndpoint* CLASS::AllocateIsochEP(void)
{
	GenericUSBXHCIIsochEP* obj = OSTypeAlloc(GenericUSBXHCIIsochEP);
	if (!obj)
		return obj;
	if (!obj->init()) {
		obj->release();
		return 0;
	}
	return obj;
}

IODMACommand* CLASS::GetNewDMACommand()
{
	return IODMACommand::withSpecification(IODMACommand::OutputHost64,
										   XHCI_HCC_AC64(_HCCLow) ? 64U : 32U,
										   0U);
}

IOReturn CLASS::GetFrameNumberWithTime(UInt64* frameNumber, AbsoluteTime* theTime)
{
	if (!frameNumber || !theTime)
		return kIOReturnBadArgument;
	if (!_commandGate)
		return kIOReturnUnsupported;
	return _commandGate->runAction(GatedGetFrameNumberWithTime, frameNumber, theTime);
}