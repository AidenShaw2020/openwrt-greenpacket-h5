'use strict';
'require view';
'require form';
'require fs';
'require network';

function parseState(text) {
	const state = {};
	for (const line of (text || '').split('\n')) {
		const i = line.indexOf('=');
		if (i > 0)
			state[line.slice(0, i)] = line.slice(i + 1);
	}
	return state;
}

return view.extend({
	load() {
		return Promise.all([
			/* rpcd checks the resolved path; /var/run links to /tmp/run */
			L.resolveDefault(fs.read('/tmp/run/h5-ippt.state'), ''),
			network.getHostHints()
		]);
	},

	render([ stateText, hosts ]) {
		const state = parseState(stateText);
		const m = new form.Map('h5ippt', _('IP Passthrough'),
			_('In router mode the H5 translates all LAN traffic to its mobile address. ' +
			  'In passthrough mode the mobile IPv4 address is leased over DHCP to one LAN router ' +
			  'and all unsolicited inbound traffic is forwarded to it, so port forwarding is ' +
			  'configured on that router only. The H5 remains reachable at its LAN address.'));

		const s = m.section(form.NamedSection, 'main', 'ippt');

		let o = s.option(form.DummyValue, '_status', _('Status'));
		o.cfgvalue = () => state.IP
			? _('Passing %s/%s (gateway %s) to %s').format(state.IP, state.PREFIX, state.GW, state.MAC)
			: _('Inactive');

		o = s.option(form.ListValue, 'enabled', _('Mode'));
		o.value('0', _('Router (NAT)'));
		o.value('1', _('IP passthrough'));
		o.default = '0';

		o = s.option(form.Value, 'mac', _('Passthrough host'),
			_('MAC address of the WAN port of the router that receives the mobile address.'));
		o.datatype = 'macaddr';
		o.depends('enabled', '1');
		o.rmempty = false;
		o.retain = true;
		/* Leases and the stored value are lower case; hints are upper case. */
		for (const [ mac, hint ] of hosts.getMACHints(false))
			o.value(mac.toLowerCase(), hint ? '%s (%s)'.format(mac, hint) : mac);
		o.write = (section_id, value) => m.data.set('h5ippt', section_id, 'mac', value.toLowerCase());

		return m.render();
	}
});
