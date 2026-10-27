'use strict';
'require view';
'require form';
'require rpc';
'require ui';
'require poll';
'require dom';

const status = rpc.declare({ object: 'h5modem', method: 'status', expect: { '': {} } });
const capabilities = rpc.declare({ object: 'h5modem', method: 'capabilities', expect: { '': {} } });
const lockLTE = rpc.declare({ object: 'h5modem', method: 'lock_lte', params: [ 'earfcn', 'pci' ], expect: { '': {} } });
const lockNR = rpc.declare({ object: 'h5modem', method: 'lock_nr', params: [ 'pci', 'arfcn', 'scs', 'band' ], expect: { '': {} } });
const unlockLTE = rpc.declare({ object: 'h5modem', method: 'unlock_lte', expect: { '': {} } });
const unlockNR = rpc.declare({ object: 'h5modem', method: 'unlock_nr', expect: { '': {} } });

function lines(data, key) {
	const q = data && data.queries && data.queries[key];
	return q && q.ok && Array.isArray(q.lines) ? q.lines : [];
}

function columns(line) {
	const body = line.slice(line.indexOf(':') + 1).trim();
	return (body.match(/"[^"]*"|[^,]+/g) || []).map(v => v.trim().replace(/^"|"$/g, ''));
}

function measurement(value) {
	return value != null && value !== '' && Number.isFinite(Number(value)) && Number(value) !== -32768 ? value : '—';
}

function table(headings, rows) {
	return E('table', { 'class': 'table cbi-section-table' }, [
		E('tr', { 'class': 'tr table-titles' }, headings.map(h => E('th', { 'class': 'th' }, h))),
		...rows.map(row => E('tr', { 'class': 'tr' }, row.map(v => E('td', { 'class': 'td' }, String(v == null ? '—' : v)))))
	]);
}

function radioContent(data) {
	if (!data || !data.ok)
		return [ E('p', {}, _('Radio information is not available yet.')) ];
	const cells = [];
	for (const line of lines(data, 'serving')) {
		const c = columns(line), lte = c.indexOf('LTE');
		if (lte >= 0 && c.length >= lte + 15)
			cells.push([ 'LTE', 'B' + c[lte + 7], c[lte + 6], c[lte + 5], measurement(c[lte + 11]), measurement(c[lte + 12]), measurement(c[lte + 14]) ]);
		else if (c[0] === 'NR5G-NSA' && c.length >= 9)
			cells.push([ '5G NSA', 'n' + c[8], c[7], c[3], measurement(c[4]), measurement(c[6]), measurement(c[5]) ]);
	}
	const result = [
		E('h3', {}, _('Serving cells')),
		table([ _('Technology'), _('Band'), _('Channel'), 'PCI', _('RSRP (dBm)'), _('RSRQ (dB)'), _('SINR (dB)') ], cells),
		E('h3', {}, _('Receive chains'))
	];
	const chains = [];
	for (const [key, label, unit] of [ [ 'rsrp', 'RSRP', 'dBm' ], [ 'sinr', 'SINR', 'dB' ] ]) {
		for (const line of lines(data, key)) {
			const c = columns(line);
			if (c.length !== 5) continue;
			chains.push([ c[4] === 'NR5G' ? '5G' : c[4], label + ' (' + unit + ')', ...c.slice(0, 4).map(measurement) ]);
		}
	}
	result.push(table([ _('Technology'), _('Measurement'), 'RX1', 'RX2', 'RX3', 'RX4' ], chains));
	return result;
}

function lockContent(data) {
	const rows = [];
	for (const [key, label] of [ [ 'lte', 'LTE' ], [ 'nr', '5G SA' ] ]) {
		const c = columns(lines(data, key)[0] || '');
		let text = _('Unknown');
		if (c[1] === '0') text = _('Not locked');
		else if (key === 'lte' && c.length >= 4) text = _('Channel %s, PCI %s').format(c[2], c[3]);
		else if (key === 'nr' && c.length >= 5) text = _('Channel %s, PCI %s, band n%s').format(c[2], c[1], c[4]);
		rows.push([label, text]);
	}
	return table([ _('Technology'), _('Current cell lock') ], rows);
}

return view.extend({
	load() { return Promise.all([ status(), capabilities() ]); },
	render(data) {
		const radio = E('div', { 'class': 'cbi-section' }, radioContent(data[0]));
		const locks = E('div', {}, [lockContent(data[1])]);
		const m = new form.JSONMap({ lock: { '.type': 'lock', technology: 'lte', earfcn: '', lte_pci: '', arfcn: '', nr_pci: '', scs: '30', band: '' } }, _('Cell lock'),
			_('5G cell locking applies to SA networks. For 5G NSA, lock the LTE anchor cell.') + ' ' +
			_('Changing a cell lock restarts network registration.'));
		const s = m.section(form.NamedSection, 'lock', 'lock');
		let o = s.option(form.ListValue, 'technology', _('Technology'));
		o.value('lte', 'LTE'); o.value('nr', '5G SA');
		for (const [name, label, rat, datatype] of [
			[ 'earfcn', 'EARFCN', 'lte', 'range(0,262143)' ],
			[ 'lte_pci', 'PCI', 'lte', 'range(0,503)' ],
			[ 'arfcn', 'NR-ARFCN', 'nr', 'range(0,3279165)' ],
			[ 'nr_pci', 'PCI', 'nr', 'range(1,1007)' ],
			[ 'band', _('NR band'), 'nr', 'range(1,1024)' ]
		]) {
			o = s.option(form.Value, name, label);
			o.depends('technology', rat); o.datatype = datatype;
		}
		o = s.option(form.ListValue, 'scs', _('Subcarrier spacing'));
		o.depends('technology', 'nr');
		for (const n of [ '15', '30', '60', '120' ]) o.value(n, n + ' kHz');
		const value = name => String(m.data.get('json', 'lock', name) || '');
		const finished = result => {
			if (!result || !result.ok) throw new Error(result && result.error || _('The modem rejected the request.'));
			ui.addNotification(null, E('p', {}, _('Cell lock updated.')), 'info');
			return capabilities().then(c => dom.content(locks, [lockContent(c)]));
		};
		const failed = error => ui.addNotification(null, E('p', {}, String(error)), 'error');
		o = s.option(form.Button, '_apply', _('Apply cell lock'));
		o.inputstyle = 'apply';
		o.onclick = () => m.save().then(() => value('technology') === 'lte'
			? lockLTE(value('earfcn'), value('lte_pci'))
			: lockNR(value('nr_pci'), value('arfcn'), value('scs'), value('band'))).then(finished).catch(failed);
		o = s.option(form.Button, '_clear', _('Remove selected cell lock'));
		o.inputstyle = 'reset';
		o.onclick = () => {
			const technology = m.lookupOption('technology', 'lock')[0].formvalue('lock');
			return (technology === 'lte' ? unlockLTE() : unlockNR()).then(finished).catch(failed);
		};
		poll.add(() => status().then(d => dom.content(radio, radioContent(d))).catch(() => {}), 10);
		return m.render().then(node => E('div', {}, [ E('h2', {}, _('Cellular Network')), radio, locks, node ]));
	},
	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
