// Copyright (c) 2011-2014 The FargoCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef E_DINAR_QT_BITCOINADDRESSVALIDATOR_H
#define E_DINAR_QT_BITCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class FargoCoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FargoCoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** FargoCoin address widget validator, checks for a valid fargocoinaddress.
 */
class FargoCoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FargoCoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // E_DINAR_QT_BITCOINADDRESSVALIDATOR_H
