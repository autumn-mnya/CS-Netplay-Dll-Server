function CaveNet.ActServer()
	-- Send packet ID 20 back, but not to the original sender.
	print("Just got ID number " .. CaveNet.eventCode())
	
	-- Weapon hell .
	if CaveNet.eventCode() > 19 or CaveNet.eventCode() < 34 then
		CaveNet.ENet.sendPacketID(CaveNet.eventCode(), false)
	end
	
	if CaveNet.specialEventCode() == 1 then
		print("Hell yeah! Custom data! gimme that")
		print(CaveNet.GetSpecialPacketData())
		CaveNet.ENet.sendSpecialPacket(1, CaveNet.GetUsername() .. CaveNet.GetPlayerID(), false, true)
	end
end