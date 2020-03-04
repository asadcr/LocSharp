package com.layer.json.codec

import java.util.UUID

import com.layer.identity.{IdentityTypes, IdentityType}
import com.layer.recon.{MetadataObject, MetadataLeaf}
import com.layer.scalazee._
import com.layer.sync.LayerMediaTypes
import com.nparry.orderly.{Orderly, Violation}
import net.liftweb.json.JsonAST.JString
import net.liftweb.json._
import spray.http.parser.HttpParser
import spray.http.{MediaType, HttpHeaders}

import scala.util.{Success, Failure, Try}

/**
 * Created by drapp on 4/3/15.
 */
trait BaseCodec {
  lazy val httpsUri = BaseCodec.httpsUri
  lazy val httpUri = BaseCodec.httpUri
  lazy val layerUri = BaseCodec.layerUri
  lazy val baseFormats = Serialization.formats(NoTypeHints) + new UUIDSerializer + new NotificationSerializer + new IdentityTypesSerializer
  val UndefinedProperty = """^property (.[^ ]) .*""".r

  class UUIDSerializer extends Serializer[UUID] {
    val uuidClass = classOf[UUID]
    def serialize(implicit format: Formats): PartialFunction[Any, JValue] = {
      case x: UUID => JString(x.toString)
    }

    def deserialize(implicit format: Formats): PartialFunction[(TypeInfo, JValue), UUID] = {
      case (TypeInfo(`uuidClass`, _), JString(x)) => UUID.fromString(x)
    }
  }

  class IdentityTypesSerializer extends Serializer[IdentityType]  {
    val identityTypeClass = classOf[IdentityType]
    def serialize(implicit format: Formats): PartialFunction[Any, JValue] = {
      case IdentityTypes.User => JString("user")
      case IdentityTypes.Bot => JString("bot")
    }

    def deserialize(implicit format: Formats): PartialFunction[(TypeInfo, JValue), IdentityType] = {
      case (TypeInfo(`identityTypeClass`, _), JString("user")) => IdentityTypes.User
      case (TypeInfo(`identityTypeClass`, _), JString("bot")) => IdentityTypes.Bot
    }
  }

  private def mkPath(parts: List[String]) = {
    val distinct = parts.distinct.filterNot(_ == "") // it lists array values with the name twice
    if(distinct.size == 1) distinct(0) else distinct.reverse.mkString(".")
  }

  def fixMessage(msg: String) = {
    // with some clean up the messages are actually pretty good
    val firstPass = msg.replaceAll("""class net.liftweb.json.JsonAST\$J""", "").replaceAll("""List\(""", "").replaceAll("""\)""", "").replaceAll("empty", "omitted")
    if(firstPass.last == '.') firstPass else s"$firstPass."
  }

  def deserialize[A](bytes: Array[Byte], orderly: Orderly)(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    deserialize(bytes, Some(orderly))
  }

  def deserialize[A](bytes: Array[Byte])(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    deserialize(bytes, None)
  }

  def deserialize[A](bytes: Array[Byte], orderly: Option[Orderly])(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    if(bytes.isEmpty) throw EmptyBodyException()
    val jsonString = new String(bytes, "UTF-8")
    val jsonAst = parse(jsonString)
    deserialize(jsonAst, orderly)
  }

  def deserialize[A](jsonAst: JValue, orderly: Orderly)(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    deserialize(jsonAst, Some(orderly))
  }

  def deserialize[A](jsonAst: JValue)(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    deserialize(jsonAst, None)
  }

  def deserialize[A](jsonAst: JValue, orderly: Option[Orderly])(implicit formats: Formats, mf: scala.reflect.Manifest[A]): A = {
    orderly.foreach {
      _.validate(jsonAst).headOption.foreach {
        case Violation(List(""), message) => throw InvalidJsonRootException(fixMessage(message))
        case Violation(path, message) if message.startsWith("missing") => throw MissingJsonPropertyException(mkPath(path))
        case Violation(path, message@UndefinedProperty(property)) => throw InvalidJsonPropertyException(property, message)
        case Violation(path, message) => throw InvalidJsonPropertyException(mkPath(path), fixMessage(message))
      }
    }
    jsonAst.extract[A]
  }

  def decompose(a: Any)(implicit formats: Formats): JValue = {
    Extraction.decompose(a)
  }

  def render(value: JValue, forceNulls: Boolean = true): Array[Byte] = {
    compactRender(if(forceNulls) value.transform { case JNothing => JNull } else value).getBytes("UTF-8")
  }

  def serialize[A](a: A, forceNulls: Boolean = true)(implicit formats: Formats): Array[Byte] = {
    render(decompose(a), forceNulls)
  }

  def handleFailure[T]: PartialFunction[Throwable, Try[T]] = {
    case e: JsonParser.ParseException => Failure(InvalidJsonException(e.getMessage))

  }

  def metadataToJson(metadataObject: MetadataObject): JObject = JObject(metadataObject.fields.toList.map {
    case (key, MetadataLeaf(string)) => JField(key, JString(string))
    case (key, subObj: MetadataObject) => JField(key, metadataToJson(subObj))
  })

  def metadataFromJson(obj: JObject, path: String): MetadataObject = MetadataObject(obj.obj.map(field => field.name -> field.value).toMap.map {
    case (name, JString(string)) => name -> MetadataLeaf(string)
    case (name, subObj: JObject) => name -> metadataFromJson(subObj, s"$path.$name")
    case (name, other) => throw new InvalidJsonPropertyException(s"$path.$name", s"Type ${humanName(other)} is not supported in metadata.")
  })

  private def humanName(value: JValue): String = value match {
    case _: JArray => "array"
    case _: JBool => "boolean"
    case _: JDouble => "double"
    case _: JField => "field"
    case _: JInt => "int"
    case _: JObject => "object"
    case _: JString => "string"
    case JNull => "null"
    case _ => ""
  }
}

object BaseCodec {
  lazy val httpsUri = "https://"
  lazy val httpUri = "http://"
  lazy val layerUri = "layer://"

  def parseVersion(mediaTypeOpt: Option[String]): Try[String] = for {
    mediaType <- mediaTypeOpt.toTry(MissingAcceptHeaderException())
    version <- HttpParser.parseHeader(HttpHeaders.RawHeader("Content-Type", mediaType)) match {
      case Right(HttpHeaders.`Content-Type`(contentType)) => parseVersion(contentType.mediaType)
      case _ => Failure(InvalidMediaTypeException(mediaType))
    }
    _ <- isSupportedVersion(version).toTryUnit(UnsupportedVersionException(s"Unsupported version: $version"))
  } yield version

  def parseVersion(mediaType: MediaType): Try[String] = mediaType match {
    case LayerMediaTypes.`application/vnd.layer.internal.version+json`(version) => Success(version)
    case _ =>
      (mediaType.mainType, mediaType.subType, mediaType.parameters.get("version")) match {
        case ("application", "vnd.layer+json", Some(version)) => Success(version)
        case _ => Failure(InvalidMediaTypeException(mediaType.toString()))
      }
  }

  def mapToJObject(map: Map[String, Any]): JObject = {
    JObject(map.toList.map { case (key, value) => JField(key, toJValue(value)) })
  }

  def jObjectToMap(obj: JObject): Map[String, Any] = {
    obj.values
  }

  private def toJValue(v: Any): JValue = v match {
    case b: Boolean =>
      JBool(b)
    case d: Double =>
      JDouble(d)
    case d: BigDecimal =>
      JDouble(d.toDouble)
    case i: Int =>
      JInt(i)
    case i: BigInt =>
      JInt(i.toInt)
    case s: String =>
      JString(s)
    case a: Seq[Any] =>
      JArray(a.toList.map(toJValue))
    case m: Map[_,_] =>
      JObject(m.toList.map { case (k, v) => JField(k.toString, toJValue(v)) })
    case null =>
      JNull
    // We may as well allow Option[Any] as map values, it came up in ScalaCheck testing
    case Some(v) =>
      toJValue(v)
    case None =>
      JNull
  }
}
