#include "function.h"
#include <QMetaMethod>
#include <QStringList>
#include "util.h"

namespace NeovimQt {

#ifndef NEOVIMQT_NO_AUTO
#include "auto/function_static.cpp"
#endif


Function::Function()
:can_fail(false), m_valid(false)
{

}

Function::Function(const QString& ret, const QString& name, QList<QPair<QString,QString> > params, bool can_fail)
:m_valid(true)
{
	this->return_type = ret;
	this->name = name;
	this->parameters = params;
	this->can_fail = can_fail;
}

Function::Function(const QString& ret, const QString& name, QList<QString> paramTypes, bool can_fail)
:m_valid(true)
{
	this->return_type = ret;
	this->name = name;
	foreach (QString type, paramTypes) {
		this->parameters .append(QPair<QString,QString>(type, ""));
	}
	this->can_fail = can_fail;
}

bool Function::isValid()
{
	return m_valid;
}

bool Function::operator==(const Function& other)
{
	if ( this->name != other.name ) {
		return false;
	}

	if ( this->can_fail != other.can_fail ) {
		qDebug() << __func__ << *this << other;
		return false;
	}

	if ( this->return_type != other.return_type ) {
		return false;
	}
	if (this->parameters.size() != other.parameters.size()) {
		return false;
	}
	for (int i=0; i<this->parameters.size(); i++) {
		if ( this->parameters.at(i).first != other.parameters.at(i).first ) {
			return false;
		}
	}
	return true;
}

Function Function::fromMsgpack(const msgpack_object& fun)
{
	Function f;
	if ( fun.type != MSGPACK_OBJECT_MAP ) {
		return f;
	}

	for (uint32_t i=0; i<fun.via.map.size; i++) {
		if ( fun.via.map.ptr[i].key.type != MSGPACK_OBJECT_RAW ) {
			return f;
		}

		QString key = toByteArray(fun.via.map.ptr[i].key);
		msgpack_object& val = fun.via.map.ptr[i].val;
		if ( key == "return_type" ) {
			if ( val.type != MSGPACK_OBJECT_RAW ) {
				return f;
			}
			f.return_type = QString::fromUtf8(val.via.raw.ptr, val.via.raw.size);
		} else if ( key == "name" ) {
			if ( val.type != MSGPACK_OBJECT_RAW ) {
				return f;
			}
			f.name = QString::fromUtf8(val.via.raw.ptr, val.via.raw.size);
		} else if ( key == "can_fail" ) {
			if ( val.type != MSGPACK_OBJECT_BOOLEAN ) {
				return f;
			}
			f.can_fail = val.via.boolean;
		} else if ( key == "parameters" ) {
			if ( val.type != MSGPACK_OBJECT_ARRAY ) {
				return f;
			}
			f.parameters = parseParameters(val);
		} else if ( key == "id" ) {
			// Deprecated
		} else if ( key == "receives_channel_id" ) {
			// Internal
		} else {
			qWarning() << "Unsupported function attribute"<< key << val.type;
		}
	}
	f.m_valid = true;
	return f;
}

/**
 * Retrieve parameter types from a list of function parameters in the metadata
 * object. Basically retrieves the even numbered elements of the array (types)
 * i.e. [Type0 name0 Type1 name1 ... ] -> [Type0 Type1 ...]
 *
 */
QList<QPair<QString,QString> > Function::parseParameters(const msgpack_object& obj)
{
	QList<QPair<QString,QString> > fail;
	if ( obj.type != MSGPACK_OBJECT_ARRAY ) {
		return fail;
	}

	QList<QPair<QString,QString> > res;
	for (uint32_t i=0; i<obj.via.array.size; i++) {
		msgpack_object& param = obj.via.array.ptr[i];
		if ( param.type != MSGPACK_OBJECT_ARRAY ) {
			return fail;
		}

		if ( param.via.array.size % 2 != 0 ) {
			return fail;
		}

		for (uint32_t j=0; j<param.via.array.size; j+=2) {
			if ( param.via.array.ptr[j].type != MSGPACK_OBJECT_RAW ||
					param.via.array.ptr[j+1].type != MSGPACK_OBJECT_RAW ) {
				return fail;
			}
			QPair<QString,QString> arg(
					toByteArray(param.via.array.ptr[j]),
					toByteArray(param.via.array.ptr[j+1])
					);
			res.append(arg);
		}
	}
	return res;
}

} // Namespace

